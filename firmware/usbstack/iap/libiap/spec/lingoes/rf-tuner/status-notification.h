#pragma once
#include <stdint.h>

/* [1] P.307 4.7.28 Command 0x17: RetStatusNotifyMask */

struct IAPRetStatusNotifyMaskPayload {
    uint8_t mask; /* IAPTunerStatusBits */
} __attribute__((packed));

struct IAPSetStatusNotifyMaskPayload {
    uint8_t mask; /* IAPTunerStatusBits */
} __attribute__((packed));

struct IAPStatusChangeNotifyPayload {
    uint8_t mask; /* IAPTunerStatusBits */
};
