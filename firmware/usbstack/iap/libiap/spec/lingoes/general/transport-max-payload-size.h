#pragma once
#include <stdint.h>

struct IAPReturnTransportMaxPayloadSizePayload {
    uint16_t max_payload_size;
} __attribute__((packed));
