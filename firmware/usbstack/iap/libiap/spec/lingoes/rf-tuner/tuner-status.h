#pragma once
#include <stdint.h>

/* [1] P.306 4.7.26 Command 0x15: RetTunerStatus */

enum IAPTunerStatusBits {
    IAPTunerStatusBits_DataReceived          = 1 << 0,
    IAPTunerStatusBits_RSSILevelChanged      = 1 << 1,
    IAPTunerStatusBits_StereoSourceIndicator = 1 << 2,
    IAPTunerStatusBits_HDSignalPresent       = 1 << 3,
    IAPTunerStatusBits_HDDigitalAudioPresent = 1 << 4,
    IAPTunerStatusBits_HDDataReceived        = 1 << 5,
};

struct IAPRetTunerStatusPayload {
    uint8_t status; /* IAPTunerStatusBits */
} __attribute__((packed));
