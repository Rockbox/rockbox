#pragma once
#include <stdint.h>

struct IAPReturnIPodSoftwareVersionPayload {
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
} __attribute__((packed));
