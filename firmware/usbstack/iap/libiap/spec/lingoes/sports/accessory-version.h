#pragma once
#include <stdint.h>

struct IAPRetAccessoryVersionPayload {
    uint8_t major;
    uint8_t minor;
} __attribute__((packed));
