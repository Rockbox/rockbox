#pragma once
#include <stdint.h>

enum IAPFIDTokenValuesScreenInfoFeatureBits {
    IAPFIDTokenValuesScreenInfoFeatureBits_ColorDisplay = 1 << 0,
};

struct IAPFIDTokenValuesScreenInfoToken {
    uint8_t  length;  /* = 0x10 */
    uint8_t  type;    /* = 0x00 */
    uint8_t  subtype; /* = 0x07 */
    uint16_t total_screen_width_inches;
    uint16_t total_screen_height_inches;
    uint16_t total_screen_width_pixels;
    uint16_t total_screen_height_pixels;
    uint16_t ipod_out_screen_width_pixels;
    uint16_t ipod_out_screen_height_pixels;
    uint8_t  screen_feature_mask; /* IAPFIDTokenValuesScreenInfoFeatureBits */
    uint8_t  screen_gamma_value;
} __attribute__((packed));

struct IAPFIDTokenValuesScreenInfoAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x07 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
} __attribute__((packed));
