#pragma once
#include <stdint.h>

struct IAPRetNumPlayingTracksPayload {
    uint32_t num_playing_tracks;
} __attribute__((packed));
