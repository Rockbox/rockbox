#pragma once
#include <stdint.h>

struct IAPRFTunerAccAckPayload {
    uint8_t status; /* = IAPAckStatus */
    uint8_t id;
} __attribute__((packed));

struct IAPRFTunerAccAckPendingPayload {
    uint8_t  status; /* = IAPAckStatus_CommandPending */
    uint8_t  id;
    uint32_t pending_time_ms;
} __attribute__((packed));
