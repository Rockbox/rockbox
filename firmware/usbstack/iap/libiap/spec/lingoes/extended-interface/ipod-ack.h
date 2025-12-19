#pragma once
#include <stdint.h>

struct IAPExtendedIPodAckPayload {
    uint8_t  status; /* IAPAckStatus */
    uint16_t id;
} __attribute__((packed));
