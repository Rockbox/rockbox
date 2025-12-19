#pragma once
#include <stdint.h>

enum IAPUIMode {
    IAPUIMode_Standard          = 0x00,
    IAPUIMode_Extended          = 0x01,
    IAPUIMode_IPodOutFullscreen = 0x02,
    IAPUIMode_IPodOutActionSafe = 0x03,
};

struct IAPSetUIModePayload {
    uint8_t ui_mode; /* IAPUIMode */
} __attribute__((packed));
