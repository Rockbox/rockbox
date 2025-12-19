#pragma once
#include <stdint.h>

/* [1] P.166 Table 3-75 iPodPreferenceToken format */
struct IAPFIDTokenValuesIPodPreferenceToken {
    uint8_t length;   /* = 0x05 */
    uint8_t type;     /* = 0x00 */
    uint8_t subtype;  /* = 0x03 */
    uint8_t class_id; /* IAPIPodPereferenceClassID */
    uint8_t setting_id;
    uint8_t restore_on_exit; /* = 0x01 */
} __attribute__((packed));

/* [1] P.171 Table 3-89 Acknowledgment format for ipodPreferenceToken */
struct IAPFIDTokenValuesIPodPreferenceAck {
    uint8_t length;   /* = 0x04 */
    uint8_t type;     /* = 0x00 */
    uint8_t subtype;  /* = 0x03 */
    uint8_t status;   /* IAPFIDTokenValuesAckStatus */
    uint8_t class_id; /* IAPIPodPereferenceClassID */
} __attribute__((packed));
