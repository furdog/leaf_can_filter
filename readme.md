# Leaf Can Filter

![alt text](./media/2026-03-30%20151818.png)
![alt text](./media/2026-03-30%20151910.png)
![alt text](./media/2026-03-30%20152030.png)
![alt text](./media/2026-03-30%20152110.png)

---

**leaf_can_filter.h**    - single header module (main logic)

**leafspy_can_filter.h** - single header module (leafspy message filtering)

---

## Leafspy messages (Reverse engineering):
Total message length: 41 bytes, Nissan Leaf (ze0) 2012,
iso-tp LBC(0x79B,0x7BB), UDS service data id (0x01):
```C
        0,     1 : (UDS service response bytes)
MSB ->  2, ... 5 : (int32_t)current_A_0 /* unknown value, not in LeafSpy */
        6,     7 : /* Unknown purpose (not found during survey) */
MSB ->  8, ... 11: (int32_t)current_A_1 /* in LeafSpy */
       12, ... 19: /* Unknown purpose (not found during survey) */
MSB -> 20,     21: (uint16_t)voltage_V /* in LeafSpy */
       22, ... 27: /* Unknown purpose (not found during survey) */
MSB -> 28,     29: (uint16_t)hx /* in LeafSpy */
       30        : /* Unknown purpose (not found during survey) */
MSB -> 31,     33: (uint24_t)soc_pct /* in LeafSpy */
       34        : /* Unknown purpose (not found during survey) */
MSB -> 35,     36: (uint16_t)ah /* in LeafSpy */
       37, ... 40: /* Unknown purpose (not found during survey) */
```

## Implementation notes
(15.04.2026, AZE0, old generation leaf)

There's an demand to add battery overcharge function (to charge more than 4.10V per cell).

I took the logs from charging session `logs/ze0_2012/plug_charge_stop_full.csv` where car fully stopped charging on its own.
I have looked into 0x1DB(HVBAT) message and what i've got so far:
- `LB_Full_CHARGE_flag` is the only significant bit that triggers HIGH for short period of time and indicates full charge.

From Dala's firmware (`https://github.com/dalathegreat/Nissan-LEAF-Battery-Upgrade`)
i have found that `LB_Full_CHARGE_flag` is responsible for correct scaling of capacity bars (i guess if there's more capacity than 24kwh)
It's triggered at startup with capacity GIDS set to full battery capacity. From my own research i have found that old leafs doesn't understand
more than 24kwh and only adjust full capacity by SOH value, which i have done in my own firmware.
But i suggest Dala solution is better.

Now i look into 0x1DC (HVBAT).
Looks like `LB_Discharge_Power_Limit` and `LB_Charge_Power_Limit` from Dala's DBC are resposible for general power limits.
It's likely recuperation could be adjusted, as well as power limit during driving. It's not linked to any other power limits like normal charging,
though can affect it (i did not test it on practice)

`LB_MAX_POWER_FOR_CHAGER` is totally related to charging power limit. It goes to maximum value of 92kwt after charging is fully done.
`LB_BPCMAX_UPRATE` goes from 111b to 000b at the same time. Other parameters didn't show anything significant. My conclusion: this is
not enough to control charging manually.

Conclusion: nothing meaningful has been found, yet. Looking for onboard charger messages instead.

(16.04.2026)

0x380 (OBC):
`Normal_Charger_Relay_Status_Flag` sets from true to false at about 25second of log, while 0x1DC message entities getting cut long before that, at around 17.5 second.

0x5BF(OBC):
`Charger_Status` is the only meaningful.

0x60 during charging (`CHARGER_STATUS_CHARGING`)
0x60 `->` 0x80 `->` 0x70 at 18.20s of the log, (`CHARGER_STATUS_CHARGING -> unknown -> unknown`)
0x70 `->` 0x78 `->` 0x30 at 21.7s of log, (`unknown -> unknown -> CHARGER_STATUS_PLUGGED_IN_TIMER_WAIT`)
0x30 remain unchanged (`CHARGER_STATUS_PLUGGED_IN_TIMER_WAIT`)

Added `research/overcharge/overcharge.gdf` graph definition for SavyCAN software to analyze specifically `logs/ze0_2012/plug_charge_stop_full.csv`

Conclusion for OBC messages: data isn't really helpful, but aligns with previous observations

I have found this issue on github "_Possible to add stop charging on Nissan Leaf?_":
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/issues/287
There is a statement that `LB_Full_CHARGE_flag` is "maybe" more gentle way for stop of charging:
```markdown
Amazing progress 👍 Yeah sorry about the typo, 0x1DB is the place to be.

By setting frame 3, bit 4 high, you indicate a full charge has been completed. Maybe this is more gentle? 'LB_Full_CHARGE_flag'

frame.data[3] = (frame.data[3] & 0xEF) | 0x10; //full charge completed

This paired with the 
frame.data[1] = (frame.data[1] & 0xE0) | 2; //request charging stop

And finally recalculating the CRC, results in a clean termination of the charge, with the charge lights shutting off.

Good that the DBC files were helpful :) 
```

Here is relevant piece of code from OVMS:
```C
    case 0x1db:
    {
      if (m_MITM>0) { // This section used to stop charging upon request
        unsigned char new1db[8];
        memcpy(new1db, d, 8); // Store current 1DB message content
        new1db[1] = (new1db[1] & 0xe0) | 2; // Charging Mode Stop Request
        new1db[6] = (new1db[6] + 1) % 4; // Increment PRUN counter
        new1db[7] = leafcrc(7, new1db); // Sign message with correct CRC
        m_can1->WriteStandard(0x1db, 8, new1db); //Send completed CAN message
        if (1==--m_MITM) MITMDisableTimer();
      }
```

This code modifies `LB_Failsafe_Status` and tells to stop the car. But in my log only `LB_Full_CHARGE_flag` is triggered.
So, it looks like we have to suppress both `LB_Full_CHARGE_flag` and later `LB_Failsafe_Status`.

I have also found that there's VCM message that is responsible for stop of charging 0x1F2(VCM) `Charge_StatusTransitionReqest`.
If that's the case, then suppression here is needed too.

Looks like VCM sets `Charge_StatusTransitionReqest` flag after `LB_Full_CHARGE_flag`, but there are some fluctuations happening in unknown data fields right before stopping. I have found that battery current also drops right before stop of charging.

> 09.06.2026

Added Isolation Resistance sensor override option.
I think that adding multiplier or other options will be `safer` option.
Currently static value of 1000mV is set, so the sensor is basically completely ignored.

> 11.06.2026

Attempt to fix soh settings, so now they change
not based on manual_capacity flag.

Also tried to add soh setting to leafspy filtering, but overriding `hx` had no effect.
Probably it stored in different group.

> 17.06.2026

Researching why 24kwh aze0 goes turtle after internal capacity goes to zero…
`ch1_20260612_CANBOX_ACTIV.csv`
`ch1_20260612_OBXID_STALA.csv`

I have found difference in usable SOC `(0x1DB, LB_Usable_SOC, byte 4 [6...0])`.
It must be set to filtered soc. (Was already done)

There also should be detection of 24kwh aze0. They don't have alternating mux value for GIDS.
CAN filter also expects full capacity available (full gids), but it never gets one, because of that.

I have found `0x5A9` (VCM, not BMS) message which:
- `LowBattery` goes from 0 to 1
- `CriticalBattery` signal goes from 0 to 1
- `ECOmodeActive` goes from 0 to 3
- `RangeInstrumentCluster` drops from 110km to 4km
The message dbc is incomplete, and there are more data. Needs further research.

`0x5b9` (also VCM):
- `ActiveFuelBars` goes from 5 to 0.
- `ChargeMinutesRemaining` is also changed

`0x5bc` (Highly confident about this):
`LB_Output_Power_Limit_Reason` is changed from 0(NORMAL) to 1(CAPACITY DROP)
`CapacityBars` changed from 15 to 7
`ChargeBars` changed from 6 to 0
`LB_Remain_Capacity_GIDS` from 201 to 6
`LB_Remaining_Capacity_Segments` muxxing changed from 240-96 to 116-0

So, here is my theory - it's either bms sets `LB_Output_Power_Limit_Reason` and we only need to fix this,
or it is VCM has it's own capacity counter, which is little more complicated to fix.
In the case of VCM i assume that sending full charge flag artifically, we can reset VCM counter.
Need to confirm that on practice...

There's also a possibility to fix old leaf charging bars scaling, based on https://github.com/dalathegreat/Nissan-LEAF-Battery-Upgrade
To fix instrumentation cluster on old leaf models:
* Between 1-3 seconds after power-on (startup_counter_1DB between 100-300ms), sets the "full charge flag" to ON
* At the same time set GIDS to full capacity
* This signals to the instrumentation cluster that the battery is at full capacity, allowing it to calibrate the scaling bars correctly
* The fix is skipped during charge sessions to avoid disrupting charging operations

> 29.6.2026

I am highly confident that `LB_Output_Power_Limit_Reason` is the reason for power limit…
Trying to fix this one.

Ok… This did not work. Lefspy still says soc is 0...

`0x55B`:
LB_SOC is in zero, but should not be. Trying to override this one…
LB_Capacity_Empty is set to true, but should be false.

---

## To-Do List
* ~~energy counter must have 0.5V/bit precision~~
* ~~override bms capacity~~
* ~~Write filesystem~~
* ~~Write web interface~~
* Eliminate race conditions (status is unknown)
* More clear separation for various vehicles
* ~~ISO-TP subset for effective can filtering of leafspy requests~~
* ISO-TP more complete version 
* Redesign WEB interface
* Enhance compatiblity between hardware variants
* Store version details based on request
* More unified interface to make CAN filter more universal
* add overcharge
* chademo power limit
* recuperation multiplier
* ~~request SOH reset~~
* ~~auto disable WiFi~~
* ~~enable WiFi by key sequence~~
* ~~English version of UI~~
* add filesystem checksum
* ~~add hints button~~
* Fix ze0 instumentation cluster

**Completion: 10/22**

---
