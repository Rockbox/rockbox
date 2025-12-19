#pragma once
#include <stdint.h>

struct IAPCreateGeniusPlaylistPayload {
    uint32_t playback_index;
} __attribute__((packed));

struct IAPIsGeniusAvailableForTrackPayload {
    uint32_t playback_index;
};
