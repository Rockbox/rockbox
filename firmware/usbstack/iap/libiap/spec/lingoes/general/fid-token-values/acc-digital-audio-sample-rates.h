#pragma once
#include <stdint.h>

struct IAPFIDTokenValuesAccDigitalAudioSampleRatesToken {
    uint8_t  length;
    uint8_t  type;    /* = 0x00 */
    uint8_t  subtype; /* = 0x0E */
    uint32_t sample_rates[];
} __attribute__((packed));

struct IAPFIDTokenValuesAccDigitalAudioSampleRatesAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x0E */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
} __attribute__((packed));
