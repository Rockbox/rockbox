#pragma once
#include <stdint.h>

struct IAPReturnIPodSerialNumPayload {
    char serial[];
} __attribute__((packed));
