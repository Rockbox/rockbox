#pragma once
#include <stdint.h>

struct IAPGetTrackArtworkDataPayload {
    uint32_t track_index;
    uint16_t format_id;
    uint32_t offset_ms;
} __attribute__((packed));

struct IAPRetTrackArtworkDataFirstPayload {
    uint16_t index;        /* = 0x0000 */
    uint8_t  pixel_format; /* IAPArtworkPixelFormats */
    uint16_t pixel_width;
    uint16_t pixel_height;
    uint16_t inset_top_left_x;
    uint16_t inset_top_left_y;
    uint16_t inset_bottom_right_x;
    uint16_t inset_bottom_right_y;
    uint32_t stride;
    uint8_t  data[];
} __attribute__((packed));

struct IAPRetTrackArtworkDataSubsequenctPayload {
    uint16_t index; /* > 0x0000 */
    uint8_t  data[];
} __attribute__((packed));
