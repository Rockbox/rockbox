#pragma once
#include <stdint.h>

/* [1] P.302 4.7.21 Command 0x10: RetTunerSeekRssi */

struct IAPRetTunerSeekRssiPayload {
    uint8_t rssi_threshold;
} __attribute__((packed));

struct IAPSetTunerSeekRssiPayload {
    uint8_t rssi_threshold;
} __attribute__((packed));
