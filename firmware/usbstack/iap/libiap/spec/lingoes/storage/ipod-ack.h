#pragma once
#include <stdint.h>

/* [1] P.359 4.11.3 Command 0x00: iPodAck */

struct IAPStorageIPodAckPayload {
    uint8_t status; /* IAPAckStatus */
    uint8_t id;
    uint8_t handle;
} __attribute__((packed));
