#pragma once
#include <stdint.h>

struct IAPStorageAccessoryAckPayload {
    uint8_t status; /* IAPAckStatus */
    uint8_t id;
    uint8_t handle;
} __attribute__((packed));
