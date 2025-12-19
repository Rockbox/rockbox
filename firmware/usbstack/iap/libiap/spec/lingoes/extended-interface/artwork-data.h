#pragma once
#include <stdint.h>

/* [1] P.459 5.1.72 Command 0x004E: GetArtworkData */

struct IAPGetArtworkDataPayload {
    uint8_t id_type; /* IAPGetArtworkTrackIDType */
    uint8_t data[];
} __attribute__((packed));

struct IAPGetArtworkDataUIDPayload {
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType_UID */
    uint8_t  uid[8];
    uint16_t format_id;
    uint32_t offset_ms;
} __attribute__((packed));

struct IAPGetArtworkDataIndexPayload {
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType_{PlaybackListIndex,DatabaseIndex} */
    uint32_t index;
    uint16_t format_id;
    uint32_t offset_ms;
} __attribute__((packed));

struct IAPRetArtworkDataPayload {
    uint16_t current_sector;
    uint16_t max_sectors;
    uint8_t  id_type; /* = IAPGetArtworkTrackIDType */
    uint8_t  data[];
} __attribute__((packed));

struct IAPRetArtworkDataBody {
    uint8_t  pixel_format; /* IAPArtworkPixelFormats */
    uint16_t pixel_width;
    uint16_t pixel_height; /* [1] P.461 "Table 5-104 imageDescriptionAndData format" misses this field, but should be here */
    uint16_t inset_top_left_x;
    uint16_t inset_top_left_y;
    uint16_t inset_bottom_right_x;
    uint16_t inset_bottom_right_y;
    uint32_t stride;
    uint8_t  data[];
} __attribute__((packed));

struct IAPRetArtworkDataUIDPayload {
    uint16_t                     current_sector;
    uint16_t                     max_sectors;
    uint8_t                      id_type; /* = IAPGetArtworkTrackIDType_UID */
    uint8_t                      uid[8];
    struct IAPRetArtworkDataBody body;
} __attribute__((packed));

struct IAPRetArtworkDataIndexPayload {
    uint16_t                     current_sector;
    uint16_t                     max_sectors;
    uint8_t                      id_type; /* = IAPGetArtworkTrackIDType_{PlaybackListIndex,DatabaseIndex} */
    uint32_t                     index;
    struct IAPRetArtworkDataBody body;
} __attribute__((packed));
