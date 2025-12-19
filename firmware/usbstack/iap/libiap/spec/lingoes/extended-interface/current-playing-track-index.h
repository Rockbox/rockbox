#pragma once
#include <stdint.h>

struct IAPReturnCurrentPlayingTrackIndexPayload {
    uint32_t index;
} __attribute__((packed));
