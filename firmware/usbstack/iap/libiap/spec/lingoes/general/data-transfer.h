#pragma once
#include <stdint.h>

struct IAPAccDataTransferPayload {
    uint16_t session_id;
    uint8_t  data[];
} __attribute__((packed));

struct IAPIPodDataTransferPayload {
    uint16_t session_id;
    uint8_t  data[];
} __attribute__((packed));
