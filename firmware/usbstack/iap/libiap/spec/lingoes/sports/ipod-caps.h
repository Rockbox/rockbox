#pragma once
#include <stdint.h>

enum IAPSportsIPodCapBits {
    Cardio   = 1 << 0,
    UserData = 1 << 1,
};

struct IAPRetIPodCapsPayload {
    uint16_t caps; /* IAPSportsIPodCapBits */
    uint8_t  user_count;
} __attribute__((packed));
