#pragma once
#include <stdint.h>

struct IAPRetPlayStatusPayload {
    uint8_t  state; /* IAPIPodStatePlayStatus */
    uint32_t track_index;
    uint32_t track_total_ms;
    uint32_t track_pos_ms;
} __attribute__((packed));
