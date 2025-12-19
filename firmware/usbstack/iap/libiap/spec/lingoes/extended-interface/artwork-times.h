#pragma once
#include <stdint.h>

/* [1] P.458 5.1.70 Command 0x004C: GetArtworkTimes */

enum IAPArtworkTrackIDType {
    IAPGetArtworkTrackIDType_UID               = 0x00,
    IAPGetArtworkTrackIDType_PlaybackListIndex = 0x01,
    IAPGetArtworkTrackIDType_DatabaseIndex     = 0x02,
};

struct IAPGetArtworkTimesPayload {
    uint8_t id_type; /* IAPGetArtworkTrackIDType */
    uint8_t data[];
} __attribute__((packed));

struct IAPGetArtworkTimesUIDPayload {
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType_UID */
    uint8_t  uid[8];
    uint16_t format_id;
    uint16_t artwork_index;
    uint16_t artwork_count;
} __attribute__((packed));

struct IAPGetArtworkTimesIndexPayload {
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType_{PlaybackListIndex,DatabaseIndex} */
    uint32_t index;
    uint16_t format_id;
    uint16_t artwork_index;
    uint16_t artwork_count;
} __attribute__((packed));

struct IAPRetArtworkTimesPayload {
    uint8_t id_type; /* IAPGetArtworkTrackIDType */
    uint8_t data[];
} __attribute__((packed));

struct IAPRetArtworkTimesUIDPayload {
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType_UID */
    uint8_t  uid[8];
    uint32_t offsets_ms[];
} __attribute__((packed));

struct IAPRetArtworkTimesIndexPayload {
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType_{PlaybackListIndex,DatabaseIndex} */
    uint32_t index;
    uint32_t offsets_ms[];
} __attribute__((packed));
