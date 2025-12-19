#pragma once
#include <stdint.h>

/* [1] P.300 4.7.18 Command 0x0D: RetTunerMode */

enum IAPTunerModeStatusBits {
    FMTunerResolution                = 3 << 0, /* IAPFMTunerResolutionID */
    TunerSeekingUp                   = 1 << 2,
    TunerSeekingWithMinRSSIThreshold = 1 << 3,
    ForceMonophonic                  = 1 << 4,
    StereoBlend                      = 1 << 5,
    FMTunerDeemphasis50usec          = 1 << 6,
    AMTunerResolution                = 1 << 7, /* IAPAMTunerResolutionID */
};

struct IAPRetTunerModePayload {
    uint8_t status; /* IAPTunerModeStatusBits */
} __attribute__((packed));

struct IAPSetTunerModePayload {
    uint8_t status; /* IAPTunerModeStatusBits */
} __attribute__((packed));
