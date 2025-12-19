#pragma once
#include <stdint.h>

struct IAPRetIPodAuthInfoPayload2p0 {
    uint8_t protocol_major; /* = 0x02 */
    uint8_t protocol_minor; /* = 0x00 */
    uint8_t cert_current_section_index;
    uint8_t cert_max_section_index;
    uint8_t cert_data[];
} __attribute__((packed));

struct IAPAckIPodAuthInfoPayload {
    uint8_t status;
} __attribute__((packed));
