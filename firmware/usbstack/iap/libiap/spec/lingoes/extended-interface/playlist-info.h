#pragma once
#include <stdint.h>

/* [1] P.456 5.1.66 Command 0x0048: GetPlaylistInfo */

enum IAPPlaylistInfoType {
    PlaylistInfo = 0x00,
};

struct IAPGetPlaylistInfoPayload {
    uint8_t  info_type; /* IAPPlaylistInfoType */
    uint32_t index;
} __attribute__((packed));

struct IAPRetPlaylistInfoPayload {
    uint8_t info_type; /* IAPPlaylistInfoType */
    uint8_t data;      /* TODO: add definitions */
} __attribute__((packed));
