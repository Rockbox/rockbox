#pragma once
#include <stdint.h>

struct IAPRetAccAuthInfoPayload {
    uint8_t protocol_major;
    uint8_t protocol_minor;
} __attribute__((packed));

struct IAPRetAccAuthInfoPayload2p0 {
    uint8_t protocol_major; /* = 0x02 */
    uint8_t protocol_minor; /* = 0x00 */
    uint8_t cert_current_section_index;
    uint8_t cert_max_section_index;
    uint8_t cert_data[];
} __attribute__((packed));

enum IAPAckAccAuthInfoStatus {
    IAPAckAccAuthInfoStatus_Supported       = 0x00,
    IAPAckAccAuthInfoStatus_CertTooLong     = 0x04,
    IAPAckAccAuthInfoStatus_Unsupported     = 0x08,
    IAPAckAccAuthInfoStatus_InvalidCert     = 0x0A,
    IAPAckAccAuthInfoStatus_InvalidCertPerm = 0x0B,
};

struct IAPAckAccAuthInfoPayload {
    uint8_t status; /* IAPAckAccAuthInfoStatus */
} __attribute__((packed));
