#pragma once
#include <stdint.h>

struct IAPSetCurrentPlayingTrackPayload {
    uint32_t index;
} __attribute__((packed));
