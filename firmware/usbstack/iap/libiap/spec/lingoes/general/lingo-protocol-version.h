#pragma once
#include <stdint.h>

struct IAPRequestLingoProtocolVersionPayload {
    uint8_t lingo;
} __attribute__((packed));

struct IAPReturnLingoProtocolVersionPayload {
    uint8_t lingo;
    uint8_t major;
    uint8_t minor;
} __attribute__((packed));
