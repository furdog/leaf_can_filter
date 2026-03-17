#pragma once

/******************************************************************************
 * Common data structure used by various parts of leaf can filter structure
 *****************************************************************************/
#include <stdint.h>

/** Generic representation of can frame */
struct leaf_can_filter_frame {
	uint32_t id;
	uint8_t  len;
	uint8_t  data[8];
};
