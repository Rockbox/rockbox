#pragma once
#include <stdint.h>

/* [1] P.457 5.1.68 Command 0x004A: PrepareUIDList */

struct IAPPrepareUIDListPayload {
    uint16_t current_sector;
    uint16_t max_sectors;
    uint8_t  uids[][8];
} __attribute__((packed));

struct IAPPlayPreparedUIDListPayload {
    uint8_t reserved;
    uint8_t uid[8];
};
