#pragma once
#include <stdint.h>

/* [1] P.167 Table 3-76 EAProtocolToken format */
struct IAPFIDTokenValuesEAProtocolToken {
    uint8_t length;
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x04 */
    uint8_t protocol_index;
    char    protocol_string[];
} __attribute__((packed));

/* [1] P.171 Table 3-90 Acknowledgment format for EAProtocolToken */
struct IAPFIDTokenValuesEAProtocolAck {
    uint8_t length;  /* = 0x04 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x04 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
    uint8_t protocol_index;
} __attribute__((packed));
