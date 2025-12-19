#pragma once
#include <stdint.h>

/* [1] P.297 4.7.12 Command 0x07: RetTunerBand */

enum IAPTunerBandStateBits {
    AMBandWorldwide = 0x00,
    FMBandEuropeUS  = 0x01,
    FMBandJapan     = 0x02,
    FNBandWide      = 0x03,
};

struct IAPRetTunerBandPayload {
    uint8_t state; /* IAPTunerBandStateBits */
} __attribute__((packed));

struct IAPSetTunerBandPayload {
    uint8_t state; /* IAPTunerBandStateBits */
} __attribute__((packed));
