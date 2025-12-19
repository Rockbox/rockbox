#pragma once
#include <stdint.h>

struct IAPCancelCommandPayload {
    uint8_t  lingo_id;
    uint16_t command_id;
    uint16_t transaction_id;
} __attribute__((packed));
