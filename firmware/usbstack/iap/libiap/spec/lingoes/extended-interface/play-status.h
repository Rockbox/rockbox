#pragma once
#include <stdint.h>

enum IAPPlayStatus {
    IAPPlayStatus_Stopped = 0x00,
    IAPPlayStatus_Playing = 0x01,
    IAPPlayStatus_Paused  = 0x02,
    IAPPlayStatus_Error   = 0xFF,
};

struct IAPExtendedRetPlayStatusPayload {
    uint32_t track_total_ms;
    uint32_t track_pos_ms;
    uint8_t  state; /* IAPPlayStatus */
} __attribute__((packed));
