#pragma once
#include <stdint.h>

struct IAPTrackNewAudioAttributesPayload {
    uint32_t sample_rate;
    uint32_t sound_check;
    uint32_t volume_adjustment;
} __attribute__((packed));
