#pragma once
#include <stdint.h>

struct IAPSetVideoDelayPayload {
    uint32_t delay_ms;
} __attribute__((packed));
