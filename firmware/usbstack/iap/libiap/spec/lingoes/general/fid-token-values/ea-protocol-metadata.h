#pragma once
#include <stdint.h>

enum IAPFIDTokenValuesEAProtocolMetadataTypes {
    IAPFIDTokenValuesEAProtocolMetadataTypes_DontFindApp               = 0x00,
    IAPFIDTokenValuesEAProtocolMetadataTypes_FindApp                   = 0x01,
    IAPFIDTokenValuesEAProtocolMetadataTypes_DontFindButDisplayMessage = 0x03,
};

struct IAPFIDTokenValuesEAProtocolMetadataToken {
    uint8_t length;  /* = 0x04 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x08 */
    uint8_t protocol_index;
    uint8_t metadata_type; /* IAPFIDTokenValuesEAProtocolMetadataTypes */
} __attribute__((packed));

struct IAPFIDTokenValuesEAProtocolMetadataAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x08 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
} __attribute__((packed));
