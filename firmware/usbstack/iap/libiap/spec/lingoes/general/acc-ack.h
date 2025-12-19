#pragma once
#include <stdint.h>

struct IAPAccAckPayload {
    uint8_t status; /* = IAPAckStatus_{Success,EBadParameter} */
    uint8_t id;
} __attribute__((packed));
