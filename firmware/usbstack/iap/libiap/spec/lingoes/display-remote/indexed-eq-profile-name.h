#pragma once
#include <stdint.h>

struct IAPGetIndexedEQProfileNamePayload {
    uint32_t index;
} __attribute__((packed));

/*
struct IAPRetIndexedEQProfileNamePayload {
    char name[];
} __attribute__((packed));
*/
