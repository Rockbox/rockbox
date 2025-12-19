#pragma once
#include <stdint.h>

struct IAPGetAccAuthSigPayload1p0 {
    uint8_t challenge[16];
    uint8_t retry;
} __attribute__((packed));

struct IAPGetAccAuthSigPayload2p0 {
    uint8_t challenge[20];
    uint8_t retry;
} __attribute__((packed));

/*
struct IAPRetAccAuthSigPayload {
    uint8_t sig[];
} __attribute__((packed));
*/

struct IAPAckAccAuthSigPayload {
    uint8_t status; /* IAPAckStatus */
} __attribute__((packed));
