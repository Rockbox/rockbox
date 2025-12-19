#pragma once
#include <stdint.h>

/* [1] P.165 Table 3-73 Accessory Info Type values */
enum IAPFIDTokenValuesAccInfoTypes {
    IAPFIDTokenValuesAccInfoTypes_AccName         = 0x01,
    IAPFIDTokenValuesAccInfoTypes_FirmwareVersion = 0x04,
    IAPFIDTokenValuesAccInfoTypes_HardwareVersion = 0x05,
    IAPFIDTokenValuesAccInfoTypes_Manufacture     = 0x06,
    IAPFIDTokenValuesAccInfoTypes_ModelNumber     = 0x07,
    IAPFIDTokenValuesAccInfoTypes_SerialNumber    = 0x08,
    IAPFIDTokenValuesAccInfoTypes_MaxPayloadSize  = 0x09,
    IAPFIDTokenValuesAccInfoTypes_AccStatus       = 0x0B,
    IAPFIDTokenValuesAccInfoTypes_RFCerts         = 0x0C,
};

struct IAPFIDTokenValuesAccInfoToken {
    uint8_t length;
    uint8_t type;      /* = 0x00 */
    uint8_t subtype;   /* = 0x02 */
    uint8_t info_type; /* IAPFIDTokenValuesAccInfoTypes */
    uint8_t info[];
} __attribute__((packed));

/* TODO: add info definitions */

struct IAPFIDTokenValuesAccInfoAck {
    uint8_t length;    /* = 0x04 */
    uint8_t type;      /* = 0x00 */
    uint8_t subtype;   /* = 0x02 */
    uint8_t status;    /* IAPFIDTokenValuesAckStatus */
    uint8_t info_type; /* IAPFIDTokenValuesAccInfoTypes */
} __attribute__((packed));
