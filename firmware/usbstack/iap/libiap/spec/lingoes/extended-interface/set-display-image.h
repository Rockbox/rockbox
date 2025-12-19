#pragma once
#include <stdint.h>

/* [1] P.435 5.1.46 Command 0x0032: SetDisplayImage */

struct IAPSetDisplayImageFirstPayload {
    uint16_t index;        /* = 0x0000 */
    uint8_t  pixel_format; /* IAPArtworkPixelFormats */
    uint16_t pixel_width;
    uint16_t pixel_height;
    uint32_t stride;
    uint8_t  data[];
} __attribute__((packed));

struct IAPSetDisplayImageSubsequenctPayload {
    uint16_t index; /* > 0x0000 */
    uint8_t  data[];
} __attribute__((packed));
