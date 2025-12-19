#pragma once
#include <stdint.h>

struct IAPRetCurrentEQIndexPayload {
    uint8_t index;
} __attribute__((packed));

struct IAPSetCurrentEQIndexPayload {
    uint8_t index;
} __attribute__((packed));
