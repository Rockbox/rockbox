#pragma once
#include <stdint.h>

/* [1] P.454 5.1.63 Command 0x0044: CreateGeniusPlaylist */

enum IAPCreateGeniusPlaylistIndexType {
    IAPCreateGeniusPlaylistIndexType_DatabaseEngine = 0x00,
    IAPCreateGeniusPlaylistIndexType_PlaybackEngine = 0x01,
};

struct IAPExtendedCreateGeniusPlaylistPayload {
    uint8_t  index_type; /* IAPCreateGeniusPlaylistIndexType */
    uint32_t index;
} __attribute__((packed));

struct IAPExtendedRefreshGeniusPlaylistPayload {
    uint32_t index;
} __attribute__((packed));

struct IAPExtendedIsGeniusAvailableForTrackPayload {
    uint8_t  index_type; /* IAPCreateGeniusPlaylistIndexType */
    uint32_t index;
} __attribute__((packed));
