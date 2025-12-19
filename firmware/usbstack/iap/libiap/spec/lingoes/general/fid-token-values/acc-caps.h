#pragma once
#include <stdint.h>

/* [1] P.164 Table 3-71 Accessory capabilities bit values */
enum IAPFIDTokenValuesAccCapsBits {
    IAPFIDTokenValuesAccCapsBits_LineOut             = 1 << 0,
    IAPFIDTokenValuesAccCapsBits_LineIn              = 1 << 1,
    IAPFIDTokenValuesAccCapsBits_VideoOut            = 1 << 2,
    IAPFIDTokenValuesAccCapsBits_USBAudioOut         = 1 << 4,
    IAPFIDTokenValuesAccCapsBits_AppsCommunication   = 1 << 9,
    IAPFIDTokenValuesAccCapsBits_CheckVolume         = 1 << 11,
    IAPFIDTokenValuesAccCapsBits_VoiceOver           = 1 << 17,
    IAPFIDTokenValuesAccCapsBits_AsyncPlaybackChange = 1 << 18,
    IAPFIDTokenValuesAccCapsBits_MixedResponse       = 1 << 19,
    IAPFIDTokenValuesAccCapsBits_AudioRoutingSwitch  = 1 << 21,
    IAPFIDTokenValuesAccCapsBits_AssistiveTouch      = 1 << 23,
};

struct IAPFIDTokenValuesAccCapsToken {
    uint8_t  length;    /* = 0x0A */
    uint8_t  type;      /* = 0x00 */
    uint8_t  subtype;   /* = 0x01 */
    uint64_t caps_bits; /* IAPFIDTokenValuesAccCapsBits */
} __attribute__((packed));

struct IAPFIDTokenValuesAccCapsAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x01 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
};
