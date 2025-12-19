#pragma once
#include <stdint.h>

/* [1] P.546 Table C-34 PlayCurrentSelection packet */

struct IAPPlayCurrentSelectionPayload {
    uint32_t track_index;
} __attribute__((packed));
