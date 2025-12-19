#pragma once
#include <stdint.h>

/* [1] P.359 4.11.5 Command 0x02: RetiPodCaps */

struct IAPIPodRetIPodCapsPayload {
    uint64_t total_space;
    uint32_t max_file_size;
    uint16_t max_write_size;
    uint8_t  reserved[6];
    uint8_t  major_version;
    uint8_t  minor_version;
} __attribute__((packed));
