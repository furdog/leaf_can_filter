#pragma once

#include "../lcf_common.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Leaf Can Filter Cells Reader */

#define LCF_CR_TX_ID 0x79Bu
#define LCF_CR_RX_ID 0x7BBu
#define LCF_CR_CELLS_COUNT 96u

enum lcf_cr_state {
	LCF_CR_STATE_IDLE,
	LCF_CR_STATE_SEND_REQUEST,
	LCF_CR_STATE_GET_RESPONSE,
	LCF_CR_STATE_FINAL
};

/** Leaf Can Filter Cells Reader */
struct lcf_cr {
	uint8_t _state;

	struct leaf_can_filter_frame _tx;
	bool			     _has_tx;
	struct leaf_can_filter_frame _rx;
	bool			     _has_rx;

	uint8_t  _cells_idx;
	uint16_t _cells_voltage_V[LCF_CR_CELLS_COUNT];

	uint8_t _group;
};

void lcf_cr_init(struct lcf_cr *self)
{
	uint8_t i;

	self->_state = LCF_CR_STATE_IDLE;

	self->_has_tx = false;
	self->_has_rx = false;

	self->_cells_idx       = 0u;
	for (i = 0u; i < LCF_CR_CELLS_COUNT; i++) {
		self->_cells_voltage_V[i] = 0u;
	}

	self->_group = 0u;
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

	if (!self->_has_rx && (f->id == LCF_CR_RX_ID)) {
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

void lcf_cr_parse_cells(struct lcf_cr *self)
{
	if (self->_rx.data[0] == 0x10) { /* First frame */
		self->_cells_idx = 0u;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[4] << 8u) | self->_rx.data[5];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[6] << 8u) | self->_rx.data[7];
		self->_cells_idx++;
	} else if ((self->_rx.data[6] == 0xFFu) &&
		   (self->_rx.data[0] == 0x2Cu)) { /* Last frame */
		self->_state = LCF_CR_STATE_FINAL;
	} else if (self->_cells_idx >= LCF_CR_CELLS_COUNT) {
		/* Check to prevent buffer overflow */
	} else if ((self->_rx.data[0] % 2u) != 0u) { /* Odd frames */
		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[1] << 8u) | self->_rx.data[2];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[3] << 8u) | self->_rx.data[4];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[5] << 8u) | self->_rx.data[6];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[7] << 8u);
	} else { /* Even frames */
		self->_cells_voltage_V[self->_cells_idx] |= self->_rx.data[1];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[2] << 8u) | self->_rx.data[3];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[4] << 8u) | self->_rx.data[5];
		self->_cells_idx++;

		self->_cells_voltage_V[self->_cells_idx] =
		    (self->_rx.data[6] << 8u) | self->_rx.data[7];
		self->_cells_idx++;
	}
}

void lcf_cr_step(struct lcf_cr *self)
{
	switch (self->_state) {
	case LCF_CR_STATE_IDLE:
		break;

	case LCF_CR_STATE_SEND_REQUEST: {
		/* (Single Frame) UDS service request code 0x21, group 2
		 * (cells) */
		uint8_t data[8] = {0x02u, 0x21u, 0x02u, 0xFFu,
				   0xFFu, 0xFFu, 0xFFu, 0xFFu};

		/* This request will force BMS to send
		 * first + consecutive frames which
		 * contains cell voltage values */
		_lcf_cr_push_tx(self, data, 8u);

		self->_state = LCF_CR_STATE_GET_RESPONSE;

		break;
	}

	case LCF_CR_STATE_GET_RESPONSE: {
		/* (FlowControl) ContinueToSend: max 1 frame,
		 * SeparationTime: 0ms */
		uint8_t data[8] = {0x30u, 0x01u, 0x00u, 0xFFu,
				   0xFFu, 0xFFu, 0xFFu, 0xFFu};

		if (!self->_has_rx) {
			break;
		}

		if (self->_rx.id != LCF_CR_RX_ID) {
			self->_has_rx = false;
			break;
		}

		if (self->_rx.len < 8u) {
			break;
		}

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
		if (self->_state == (uint8_t)LCF_CR_STATE_FINAL) {
			lcf_cr_init(self);
		}

		break;
	}

	default:
		break;
	}
}
