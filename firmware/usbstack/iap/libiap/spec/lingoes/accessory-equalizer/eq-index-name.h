#pragma once
#include <stdint.h>

struct IAGetEQIndexNamePayload {
    uint8_t index;
} __attribute__((packed));

struct IARetEQIndexNamePayload {
    uint8_t index;
    char    name[];
} __attribute__((packed));
