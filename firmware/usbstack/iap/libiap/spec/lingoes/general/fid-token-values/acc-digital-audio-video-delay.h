#pragma once
#include <stdint.h>

struct IAPFIDTokenValuesAccDigitalAudioVideoDelayToken {
    uint8_t  length;  /* = 0x06 */
    uint8_t  type;    /* = 0x00 */
    uint8_t  subtype; /* = 0x0F */
    uint32_t delay;
} __attribute__((packed));

struct IAPFIDTokenValuesAccDigitalAudioVideoDelayAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x0F */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
} __attribute__((packed));
