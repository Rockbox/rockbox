#pragma once
#include <stdint.h>

/* [1] P.440 5.1.48 Command 0x0034: ReturnMonoDisplayImageLimits */

struct IAPReturnMonoDisplayImageLimitsPayload {
    uint16_t max_width;
    uint16_t max_height;
    uint8_t  pixel_format; /* IAPArtworkPixelFormats */
} __attribute__((packed));
