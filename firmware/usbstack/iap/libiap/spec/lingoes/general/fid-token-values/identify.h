#pragma once
#include <stdint.h>

struct IAPFIDTokenValuesIdentifyTokenHead {
    uint8_t length;
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x00 */
    uint8_t num_lingoes;
} __attribute__((packed));

struct IAPFIDTokenValuesIdentifyTokenTail {
    uint32_t device_option; /* = 0b10 */
    uint32_t device_id;
} __attribute__((packed));

struct IAPFIDTokenValuesIdentifyToken {
    struct IAPFIDTokenValuesIdentifyTokenHead head;
    /*uint8_t lingoes[num_lingoes]; */
    struct IAPFIDTokenValuesIdentifyTokenHead tail;
} __attribute__((packed));

struct IAPFIDTokenValuesIdentifyAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x00 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
    uint8_t lingo_ids[];
} __attribute__((packed));
