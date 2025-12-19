#pragma once
#include <stdint.h>

/* [1] P.360 4.11.7 Command 0x07: WriteiPodFileData */

struct IAPWriteIPodFileDataPayload {
    uint32_t offset;
    uint8_t  handle;
    uint8_t  data[];
} __attribute__((packed));
