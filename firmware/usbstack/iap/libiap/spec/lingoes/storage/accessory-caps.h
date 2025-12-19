#pragma once
#include <stdint.h>

/* [1] P.365 4.11.14 Command 0x82: RetAccessoryCaps */

struct IAPStorageRetAccessoryCapsPayload {
    uint8_t reserved1; /* = 0x00 */
    uint8_t reserved2; /* = 0xFF */
    uint8_t major;
    uint8_t minor;
} __attribute__((packed));
