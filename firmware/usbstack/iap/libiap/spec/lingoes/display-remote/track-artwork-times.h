#pragma once
#include <stdint.h>

struct IAPGetTrackArtworkTimesPayload {
    uint32_t track_index;
    uint16_t format_id;
    uint16_t artwork_index;
    uint16_t artwork_count;
} __attribute__((packed));

/*
struct IAPRetTrackArtworkTimesPayload {
    uint32_t offsets_ms[];
} __attribute__((packed));
*/
