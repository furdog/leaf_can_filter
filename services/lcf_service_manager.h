/* TODO finish + tests */

#include "lcf_cells_reader.h"
#include "lcf_soh_reset.h"

/** Read cell voltages every 10s */
#define LCF_CR_READ_INTERVAL_MS 10000u

/** How long LeafSpy not be present on the bus to be considered inactive */
#define LCF_LEAFSPY_MAX_INACTIVITY_TIME_MS 10000u

struct lcf_service_manager {
	uint8_t _iso_tp_srv_state; /* Manages iso_tp services */

	/* Leafspy monitoring */
	bool	 leafspy_activity_detected;
	uint32_t leafspy_inactivity_timer_ms;

	/* Various services */
	struct lcf_sr soh_rst_fsm;
	struct lcf_cr cells_r_fsm;

	uint32_t _cells_read_timer_ms;
};

void lcf_service_manager_init(struct lcf_service_manager *self)
{
	self->_iso_tp_srv_state = 0u;

	/* Leafspy monitoring */
	self->leafspy_activity_detected	  = false;
	self->leafspy_inactivity_timer_ms = 0u;

	/* Various services */
	lcf_sr_init(&self->soh_rst_fsm);
	lcf_cr_init(&self->cells_r_fsm);

	self->_cells_read_timer_ms = 0u;
}

/** This will push frame (to various services).
 *  Returns true on success */
bool lcf_service_manager_push_frame(struct lcf_service_manager	 *self,
				    struct leaf_can_filter_frame *frame)
{
	bool success = false;

	/* Always try to detect leafspy activity */
	if (frame->id == 0x79Bu) {
		self->leafspy_activity_detected	  = true;
		self->leafspy_inactivity_timer_ms = 0u;

		/* Every iso-tp routine should be stopped */
		self->_iso_tp_srv_state = 3u;
	}

	if (lcf_cr_push_rx_frame(&self->cells_r_fsm, frame)) {
		success = true;
	}

	if (lcf_sr_push_frame(&self->soh_rst_fsm, frame)) {
		success = true;
	}

	return success;
}

/** This will pop generated frame (by various services).
 *  Returns true if generated any frame */
bool lcf_service_manager_pop_frame(struct lcf_service_manager	*self,
				   struct leaf_can_filter_frame *frame)
{
	bool success = false;

	if (!self->leafspy_activity_detected) {
		if (lcf_cr_pop_tx_frame(&self->cells_r_fsm, frame)) {
			success = true;
		} else if (lcf_sr_pop_frame(&self->soh_rst_fsm, frame)) {
			success = true;
		} else {
		}
	}

	return success;
}

void lcf_service_manager_update(struct lcf_service_manager *self,
				uint32_t		    delta_time_ms)
{
	/* Run and update ISO TP services */
	switch (self->_iso_tp_srv_state) {
	/* Check if any services has been awaken */
	default:
	case 0u:
		if (lcf_sr_is_awake(&self->soh_rst_fsm)) {
			lcf_sr_step(&self->soh_rst_fsm, 0u);
			self->_iso_tp_srv_state = 1u;
			break;
		}

		/* TODO prevent starvation of this service in the worst case */
		if (lcf_cr_is_awake(&self->cells_r_fsm)) {
			(void)lcf_cr_step(&self->cells_r_fsm, 0u);
			self->_iso_tp_srv_state = 2u;
			break;
		}

		break;

	/* Run SOH RESET service*/
	case 1u:
		lcf_sr_step(&self->soh_rst_fsm, delta_time_ms);

		/* Go back to checkin */
		if (!lcf_sr_is_awake(&self->soh_rst_fsm)) {
			self->_iso_tp_srv_state = 0u;
		}

		break;

	/* Run READ CELLS service*/
	case 2u:
		(void)lcf_cr_step(&self->cells_r_fsm, delta_time_ms);

		/* Go back to checkin */
		if (!lcf_cr_is_awake(&self->cells_r_fsm)) {
			self->_iso_tp_srv_state = 0u;
		}

		break;

	/* Leafspy Activity Has been detected - stop all the activity */
	case 3u:
		lcf_sr_stop(&self->soh_rst_fsm);

		/* This could work in passive state,
		 * instead of full stop (TODO) */
		lcf_cr_stop(&self->cells_r_fsm);

		self->_iso_tp_srv_state = 0u;
		break;
	}

	/* Monitor leafspy inactivity time */
	if (self->leafspy_inactivity_timer_ms <
	    LCF_LEAFSPY_MAX_INACTIVITY_TIME_MS) {
		self->leafspy_inactivity_timer_ms += delta_time_ms;
	} else {
		self->leafspy_activity_detected = false;
	}

	/* Call CELLS READER service every 10s */
	if (self->_cells_read_timer_ms < LCF_CR_READ_INTERVAL_MS) {
		self->_cells_read_timer_ms += delta_time_ms;
	} else {
		self->_cells_read_timer_ms = 0u;

		lcf_cr_start(&self->cells_r_fsm);
	}
}
