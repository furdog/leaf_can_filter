#pragma once

#include "../lcf_common.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Leaf Can Filter Cells Reader */

#define LCF_CR_TX_ID 0x79Bu
#define LCF_CR_RX_ID 0x7BBu
#define LCF_CR_CELLS_COUNT 96u
#define LCF_CR_RESPONSE_TIMEOUT_MS 5000u /* Max 5s timeout */

enum lcf_cr_event {
	LCF_CR_EVENT_NONE,
	LCF_CR_EVENT_CELLS_READY,
	LCF_CR_EVENT_TIMEOUT
};

enum lcf_cr_state {
	LCF_CR_STATE_IDLE,
	LCF_CR_STATE_SEND_REQUEST,
	LCF_CR_STATE_GET_RESPONSE
};

/** Leaf Can Filter Cells Reader */
struct lcf_cr {
	uint8_t _state;

	struct leaf_can_filter_frame _tx;
	bool			     _has_tx;
	struct leaf_can_filter_frame _rx;
	bool			     _has_rx;

	uint8_t	 _cells_idx;
	uint16_t _cell_voltages_mV[LCF_CR_CELLS_COUNT];

	uint16_t _min_cell_voltage_mV;
	uint16_t _max_cell_voltage_mV;

	uint8_t _group;

	uint32_t _timer_ms;
};

void lcf_cr_init(struct lcf_cr *self)
{
	uint8_t i;

	self->_state = LCF_CR_STATE_IDLE;

	self->_has_tx = false;
	self->_has_rx = false;

	self->_cells_idx = 0u;
	for (i = 0u; i < LCF_CR_CELLS_COUNT; i++) {
		self->_cell_voltages_mV[i] = 0u;
	}

	self->_min_cell_voltage_mV = (uint16_t)-1;
	self->_max_cell_voltage_mV = 0u;

	self->_group = 0u;

	self->_timer_ms = 0u;
}

void _lcf_cr_push_tx(struct lcf_cr *self, uint8_t *data, uint8_t len)
{
	/* Prepare TX frame for popping */
	self->_has_tx = true;
	self->_tx.id  = LCF_CR_TX_ID;
	self->_tx.len = len;
	(void)memcpy(self->_tx.data, data, 8u);
}

/** Push RX CAN frame for processing, returns false if busy. */
bool lcf_cr_push_rx_frame(struct lcf_cr *self, struct leaf_can_filter_frame *f)
{
	bool success = false;

	if (!self->_has_rx) {
		self->_has_rx = true;

		self->_rx = *f;

		success = true;
	}

	return success;
}

/** Pop TX CAN frame, returns false if busy. */
bool lcf_cr_pop_tx_frame(struct lcf_cr *self, struct leaf_can_filter_frame *f)
{
	bool result = false;

	if (self->_has_tx) {
		self->_has_tx = false;

		if (f != NULL) {
			*f = self->_tx;
		}

		result = true;
	}

	return result;
}

/** Start FSM to request cell voltages. (previous result is discarded) */
void lcf_cr_start(struct lcf_cr *self)
{
	lcf_cr_init(self);

	self->_state = LCF_CR_STATE_SEND_REQUEST;
}

/** Stop FSM. (previous result is discarded) */
void lcf_cr_stop(struct lcf_cr *self)
{
	lcf_cr_init(self);

	self->_state = LCF_CR_STATE_IDLE;
}

/** The service has been awaken due to input and requires attention */
bool lcf_cr_is_awake(struct lcf_cr *self)
{
	return self->_state != (uint8_t)LCF_CR_STATE_IDLE;
}

/** Get pointer to cell voltages.
 *  Returns NULL if invalid.
 *  Always run this function before reading cell values.
 *  TODO stricter access control to prevent use of invalid buffer */
uint16_t *lcf_cr_get_cells_mV(struct lcf_cr *self)
{
	uint16_t *result = NULL;

	if ((self->_cells_idx == LCF_CR_CELLS_COUNT) &&
	    (self->_state == (uint8_t)LCF_CR_STATE_IDLE)) {
		result = self->_cell_voltages_mV;
	}

	return result;
}

/** Get min/max cell voltages. Returns true on success. */
bool lcf_cr_get_cells_minmax_mV(struct lcf_cr *self, uint16_t *min_cell_mV,
			 uint16_t *max_cell_mV)
{
	bool success = false;

	if ((self->_cells_idx == LCF_CR_CELLS_COUNT) &&
	    (self->_state == (uint8_t)LCF_CR_STATE_IDLE)) {
		*min_cell_mV = self->_min_cell_voltage_mV;
		*max_cell_mV = self->_max_cell_voltage_mV;

		success = true;
	}

	return success;
}

/** Update min/max cell based on current _cells_idx value */
void _lcf_cr_calc_minmax(struct lcf_cr *self)
{
	uint16_t voltage_mV = self->_cell_voltages_mV[self->_cells_idx];

	if (voltage_mV > self->_max_cell_voltage_mV) {
		self->_max_cell_voltage_mV = voltage_mV;
	}

	if (voltage_mV < self->_min_cell_voltage_mV) {
		self->_min_cell_voltage_mV = voltage_mV;
	}
}

/** Parse and append data (uint16_t) from CAN message to cell data.
 *  Specify HI and LO byte offsets within CAN message to parse */
void _lcf_cr_append_cell(struct lcf_cr *self, uint8_t hi_ofs, uint8_t lo_ofs)
{
	assert(self && (hi_ofs < 8u) && (lo_ofs < 8u));

	/* Always perform check to prevent buffer overflow */
	if (self->_cells_idx < LCF_CR_CELLS_COUNT) {
		self->_cell_voltages_mV[self->_cells_idx] =
		    (self->_rx.data[hi_ofs] << 8u) | self->_rx.data[lo_ofs];

		_lcf_cr_calc_minmax(self);

		self->_cells_idx++;
	}
}

/** TODO parse raw buffer according to ISO-TP to simplify this routine */
void lcf_cr_parse_cells(struct lcf_cr *self)
{
	if (self->_rx.data[0] == 0x10u) { /* First frame */
		self->_cells_idx = 0u;
		_lcf_cr_append_cell(self, 4u, 5u);
		_lcf_cr_append_cell(self, 6u, 7u);
	} else if ((self->_cells_idx >= LCF_CR_CELLS_COUNT) &&
		   (self->_rx.data[0] == 0x2Cu)) { /* Last frame */
		self->_state = LCF_CR_STATE_IDLE;
	} else if (self->_cells_idx >= LCF_CR_CELLS_COUNT) {
		/* Check to prevent buffer overflow */
	} else if ((self->_rx.data[0] % 2u) != 0u) { /* Odd frames */
		_lcf_cr_append_cell(self, 1u, 2u);
		_lcf_cr_append_cell(self, 3u, 4u);
		_lcf_cr_append_cell(self, 5u, 6u);

		/* Special case (partial write start) */
		if (self->_cells_idx < LCF_CR_CELLS_COUNT) {
			self->_cell_voltages_mV[self->_cells_idx] =
			    (self->_rx.data[7] << 8u);
		}
	} else { /* Even frames */
		/* Special case (partial write end) */
		if (self->_cells_idx < LCF_CR_CELLS_COUNT) {
			self->_cell_voltages_mV[self->_cells_idx] |=
			    self->_rx.data[1];

			_lcf_cr_calc_minmax(self);

			self->_cells_idx++;
		}

		_lcf_cr_append_cell(self, 2u, 3u);
		_lcf_cr_append_cell(self, 4u, 5u);
		_lcf_cr_append_cell(self, 6u, 7u);
	}
}

enum lcf_cr_event lcf_cr_step(struct lcf_cr *self, uint32_t delta_time_ms)
{
	enum lcf_cr_event ev = LCF_CR_EVENT_NONE;

	if (self->_state == (uint8_t)LCF_CR_STATE_SEND_REQUEST) {
		/* (Single Frame) UDS service request code 0x21, group 2
		 * (cells) */
		uint8_t data[8] = {0x02u, 0x21u, 0x02u, 0xFFu,
				   0xFFu, 0xFFu, 0xFFu, 0xFFu};

		/* This request will force BMS to send
		 * first + consecutive frames which
		 * contains cell voltage values */
		_lcf_cr_push_tx(self, data, 8u);

		self->_state = LCF_CR_STATE_GET_RESPONSE;

		/* Clean state before listening */
		self->_has_rx	= false;
		self->_group	= 0u;
		self->_timer_ms = 0u;
	}

	/* Check timeout */
	if (self->_state == (uint8_t)LCF_CR_STATE_GET_RESPONSE) {
		self->_timer_ms += delta_time_ms;

		if (self->_timer_ms >= LCF_CR_RESPONSE_TIMEOUT_MS) {
			lcf_cr_stop(self);

			ev = LCF_CR_EVENT_TIMEOUT;
		}
	}

	/* If awaiting response, and have a rx, and len <= 8 and id match */
	if ((self->_state == (uint8_t)LCF_CR_STATE_GET_RESPONSE) &&
	    self->_has_rx && (self->_rx.len <= 8u) &&
	    (self->_rx.id == LCF_CR_RX_ID)) {
		/* (FlowControl) ContinueToSend: max 1 frame,
		 * SeparationTime: 0ms */
		uint8_t data[8] = {0x30u, 0x01u, 0x00u, 0xFFu,
				   0xFFu, 0xFFu, 0xFFu, 0xFFu};

		/* Extract group ID from first message */
		if (self->_rx.data[0] == 0x10u) {
			self->_group = self->_rx.data[3];
		}

		/* Check if extracted group matches cells group */
		if (self->_group == 0x02u) {
			/* Parse cells data */
			lcf_cr_parse_cells(self);

			/* Request next frame */
			_lcf_cr_push_tx(self, data, 8u);
		}

		/* If state switched to final */
		if (self->_state == (uint8_t)LCF_CR_STATE_IDLE) {
			ev = LCF_CR_EVENT_CELLS_READY;
		}
	}

	/* Always Reset RX flag */
	self->_has_rx = false;

	return ev;
}
