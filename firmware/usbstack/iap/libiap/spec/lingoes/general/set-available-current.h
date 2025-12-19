#pragma once
#include <stdint.h>

struct IAPSetAvailableCurrentPayload {
    uint16_t current_limit_ma;
} __attribute__((packed));
