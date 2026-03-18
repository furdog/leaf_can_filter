/*
 Total len  First byte
	 |  |     Cells data first byte
	 |  |     |
0x7BB 10 C6 61 02 FF FF FF FF   6
0x7BB 21 FF FF FF FF FF FF FF   13
0x7BB 22 FF FF FF FF FF FF FF   20
0x7BB 23 FF FF FF FF FF FF FF   27
0x7BB 24 FF FF FF FF FF FF FF   34
0x7BB 25 FF FF FF FF FF FF FF   41
0x7BB 26 FF FF FF FF FF FF FF   48
0x7BB 27 FF FF FF FF FF FF FF   55
0x7BB 28 FF FF FF FF FF FF FF   62
0x7BB 29 FF FF FF FF FF FF FF   69
0x7BB 2A FF FF FF FF FF FF FF   76
0x7BB 2B FF FF FF FF FF FF FF   83
0x7BB 2C FF FF FF FF FF FF FF   90
0x7BB 2D FF FF FF FF FF FF FF   97
0x7BB 2E FF FF FF FF FF FF FF   104
0x7BB 2F FF FF FF FF FF FF FF   111
0x7BB 20 FF FF FF FF FF FF FF   118
0x7BB 21 FF FF FF FF FF FF FF   125
0x7BB 22 FF FF FF FF FF FF FF   132
0x7BB 23 FF FF FF FF FF FF FF   139
0x7BB 24 FF FF FF FF FF FF FF   146
0x7BB 25 FF FF FF FF FF FF FF   153
0x7BB 26 FF FF FF FF FF FF FF   160
0x7BB 27 FF FF FF FF FF FF FF   167
0x7BB 28 FF FF FF FF FF FF FF   174
0x7BB 29 FF FF FF FF FF FF FF   181
0x7BB 2A FF FF FF FF FF FF FF   188
0x7BB 2B FF FF FF FF FF FF 00   195
			 |
			Cells data last byte
0x7BB 2C 00 00 00 FF FF FF FF   198
		| Padding bytes
		|
		198 - C6 (last byte)
*/

#include "lcf_cells_reader.h"
#include <stdio.h>

struct example_frame {
	double	 timestamp_s;
	uint32_t id;
	uint8_t	 dlc;
	uint8_t	 data[8u];
};

#define EXAMPLE_FRAMES_MAX 30u

struct example_frame example_frames[EXAMPLE_FRAMES_MAX] = {
    /* Dummy frame */
    {0.000,
     0x0000079Bu, 8u,
     {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u}},

    /* First frame */
    {0.000,
     0x7BBu, 8u,
     {0x10u, 0xC6u, 0x61u, 0x02u, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},

    /* Consecutive frames */
    {0.000,
     0x7BBu, 8u,
     {0x21u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x22u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x23u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x24u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x25u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x26u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x27u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x28u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x29u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Au, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Bu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Cu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Du, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Eu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Fu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x20u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x21u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x22u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x23u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x24u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x25u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x26u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x27u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x28u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x29u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Au, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}},
    {0.000,
     0x7BBu, 8u,
     {0x2Bu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0xAAu, 0x00u}}, /* last: 170 */
    {0.000,
     0x7BBu, 8u,
     {0x2Cu, 0x00u, 0x00u, 0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu}}
};

struct lcf_cr cells_reader;
uint8_t	      frame_counter = 0u;
bool	      test_timeout  = false;

void _lcf_cr_test_get_next_rx_frame(struct leaf_can_filter_frame *f)
{
	assert(frame_counter < EXAMPLE_FRAMES_MAX);

	f->id  = example_frames[frame_counter].id;
	f->len = example_frames[frame_counter].dlc;
	memcpy(f->data, example_frames[frame_counter].data, f->len);

	frame_counter++;
}

void lcf_cr_test_example_log(struct lcf_cr *self)
{
	uint8_t		  i;
	enum lcf_cr_event last_event = LCF_CR_EVENT_NONE;

	uint8_t req[8] = {0x02u, 0x21u, 0x02u, 0xFFu,
			  0xFFu, 0xFFu, 0xFFu, 0xFFu};

	struct leaf_can_filter_frame f;

	/* lcf_cr_init(self); */
	frame_counter = 0u;

	/* No frames should be generated before start */
	assert(lcf_cr_pop_tx_frame(self, &f) == false);

	lcf_cr_start(self);
	lcf_cr_step(self, 0u);

	/* Request frame should be generated after start */
	assert(lcf_cr_pop_tx_frame(self, &f) == true);
	assert(f.id == LCF_CR_TX_ID);
	assert(memcmp(f.data, req, f.len) == 0);

	/* No frames should remain after popped request frame */
	assert(lcf_cr_pop_tx_frame(self, &f) == false);

	assert(self->_cells_idx == 0);

	/* Only one push at a time is allowed (Skip dummy frame) */
	_lcf_cr_test_get_next_rx_frame(&f);
	assert(lcf_cr_push_rx_frame(self, &f) == true);
	assert(lcf_cr_push_rx_frame(self, &f) == false);
	lcf_cr_step(self, 0u);

	/* Push first frame */
	_lcf_cr_test_get_next_rx_frame(&f);
	assert(lcf_cr_push_rx_frame(self, &f) == true);
	lcf_cr_step(self, 0u);

	/* Check we've got more cells read after first message */
	printf("self->_cells_idx: %u\n", self->_cells_idx);
	assert(self->_cells_idx == 2u);

	for (i = 0; i < 100u; i++) {
		if (self->_cells_idx >= LCF_CR_CELLS_COUNT) {
			break;
		}

		assert(last_event == LCF_CR_EVENT_NONE);

		/* Push rest of the frames */
		_lcf_cr_test_get_next_rx_frame(&f);
		assert(lcf_cr_push_rx_frame(self, &f) == true);
		last_event = lcf_cr_step(self, 0u);
	}

	printf("self->_cells_idx: %u\n", self->_cells_idx);

	assert(self->_cells_idx == LCF_CR_CELLS_COUNT);
	assert(last_event != LCF_CR_EVENT_CELLS_READY);

	/* We stil should not have cells ready at this point */
	assert(lcf_cr_get_cells_raw(self) == NULL);

	/* Push one more frame to receive last one */
	_lcf_cr_test_get_next_rx_frame(&f);
	assert(lcf_cr_push_rx_frame(self, &f) == true);
	assert(lcf_cr_step(self, 0u) == LCF_CR_EVENT_CELLS_READY);

	/* We must now have pointer to cells */
	assert(lcf_cr_get_cells_raw(self) != NULL);

	/*for (i = 0; i < LCF_CR_CELLS_COUNT; i++) {
		printf("self->_cell_voltages_mV[%u]: %u\n", i,
		       self->_cell_voltages_mV[i]);
	}*/

	assert(self->_cell_voltages_mV[95] == 170u);

	/* Just run for one more time */
	assert(lcf_cr_step(self, 0u) == LCF_CR_EVENT_NONE);

	/* Pop whatever TX we have */
	assert(lcf_cr_pop_tx_frame(self, &f) == true);

	/* Fail assert */
	/* _lcf_cr_test_get_next_rx_frame(&f); */
}

void lcf_cr_test_timeout(struct lcf_cr *self)
{
	frame_counter = 0u;

	/* Nah */
	assert(lcf_cr_step(self, LCF_CR_RESPONSE_TIMEOUT_MS - 1u) ==
	       LCF_CR_EVENT_NONE);
	assert(lcf_cr_step(self, 1u) == LCF_CR_EVENT_NONE);

	lcf_cr_start(self);
	lcf_cr_step(self, 0u);

	/* Yah */
	assert(lcf_cr_step(self, LCF_CR_RESPONSE_TIMEOUT_MS - 1u) ==
	       LCF_CR_EVENT_NONE);
	assert(lcf_cr_step(self, 1u) == LCF_CR_EVENT_TIMEOUT);
}

int main()
{
	lcf_cr_init(&cells_reader);

	/* Run twice to confirm stability */
	lcf_cr_test_example_log(&cells_reader);
	lcf_cr_test_example_log(&cells_reader);

	lcf_cr_test_timeout(&cells_reader);

	return 0;
}
