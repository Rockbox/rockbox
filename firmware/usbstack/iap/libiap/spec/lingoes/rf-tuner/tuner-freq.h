#pragma once
#include <stdint.h>

/* [1] P.298 4.7.15 Command 0x0A: RetTunerFreq */

struct IAPRetTunerFreqPayload {
    uint32_t freq_khz;
    uint8_t  rssi_level;
} __attribute__((packed));

struct IAPSetTunerFreqPayload {
    uint32_t freq_khz;
} __attribute__((packed));
