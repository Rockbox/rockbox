#pragma once
#include <stdint.h>

/* [1] P.314 4.7.36 Command 0x1F: RetRdsNotifyMask */

struct IAPRetRdsNotifyMaskPayload {
    uint32_t mask; /* 1 << IAPRdsData{Parsed,Raw}Type | ... */
} __attribute__((packed));

struct IAPSetRdsNotifyMaskPayload {
    uint32_t mask; /* 1 << IAPRdsData{Parsed,Raw}Type | ... */
} __attribute__((packed));
