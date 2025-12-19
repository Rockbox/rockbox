#pragma once
#include <stdint.h>

struct IAPOpenDataSessionForProtocolPayload {
    uint16_t session_id;
    uint8_t protocol_index;
} __attribute__((packed));

struct IAPCloseDataSessionPayload {
    uint16_t session_id;
} __attribute__((packed));
