#pragma once
#include <stdint.h>

enum IAPArtworkPixelFormats {
    IAPArtworkPixelFormats_Mono     = 0x01,
    IAPArtworkPixelFormats_RGB565LE = 0x02,
    IAPArtworkPixelFormats_RGB565BE = 0x03,
};

struct IAPArtworkFormat {
    uint16_t format_id;
    uint8_t  pixel_format; /* IAPArtworkPixelFormats */
    uint16_t image_width;
    uint16_t image_height;
} __attribute__((packed));

/*
struct IAPRetArtworkFormatsPayload {
    struct IAPArtworkFormat formats[];
} __attribute__((packed));
*/
