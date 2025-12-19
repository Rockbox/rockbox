#pragma once
#include <stdint.h>

struct IAPRetCurrentEQProfileIndexPayload {
    uint32_t index;
} __attribute__((packed));

struct IAPSetCurrentEQProfileIndexPayload {
    uint32_t index;
    uint8_t  restore_on_exit;
} __attribute__((packed));
