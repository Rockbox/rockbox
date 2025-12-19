#pragma once
#include <stdint.h>

struct IAPFIDTokenValuesBundleSeedIDPrefToken {
    uint8_t length;  /* = 0x0D */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x05 */
    char    bundle_seed_id_string[11];
} __attribute__((packed));

struct IAPFIDTokenValuesBundleSeedIDPrefAck {
    uint8_t length;  /* = 0x03 */
    uint8_t type;    /* = 0x00 */
    uint8_t subtype; /* = 0x05 */
    uint8_t status;  /* IAPFIDTokenValuesAckStatus */
} __attribute__((packed));
