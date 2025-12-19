#pragma once
#include <stdint.h>

enum IAPSportsAccessoryCapBits {
    IAPAccessoryCapBits_LaterCommands = 1 << 9,
};

struct IAPRetAccessoryCapsPayload {
    uint16_t mask;     /* IAPSportsAccessoryCapBits */
    uint8_t  reserved; /* = 0x00 */
} __attribute__((packed));
