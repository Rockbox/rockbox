#pragma once
#include <stdint.h>

/* [1] P.362 4.11.10 Command 0x11: RetiPodFreeSpace */

struct IAPRetIPodFreeSpacePayload {
    uint64_t free_space;
} __attribute__((packed));
