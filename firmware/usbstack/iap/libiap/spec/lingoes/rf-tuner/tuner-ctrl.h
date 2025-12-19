#pragma once
#include <stdint.h>

/* [1] P.294 4.7.9 Command 0x04: RetTunerCtrl */

enum IAPTunerCtrlStateBits {
    PowerDraw               = 1 << 0,
    StateChangeNotification = 1 << 1,
    RDSRawMode              = 1 << 3,
};

struct IAPRetTunerCtrlPayload {
    uint8_t state; /* IAPTunerCtrlStateBits */
} __attribute__((packed));

struct IAPSetTunerCtrlPayload {
    uint8_t state; /* IAPTunerCtrlStateBits */
} __attribute__((packed));
