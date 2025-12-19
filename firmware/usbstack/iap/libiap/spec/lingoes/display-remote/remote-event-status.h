#pragma once
#include <stdint.h>

struct IAPRetRemoteEventStatusPayload {
    uint32_t mask; /* (1 << IAPRemoteEventNotifications) | ... */
} __attribute__((packed));
