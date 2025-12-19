#pragma once
#include <stdint.h>

struct IAPRequestApplicationLaunchPayload {
    uint8_t reserved1;
    uint8_t flag;
    uint8_t reserved2;
    char    app_id[];
} __attribute__((packed));
