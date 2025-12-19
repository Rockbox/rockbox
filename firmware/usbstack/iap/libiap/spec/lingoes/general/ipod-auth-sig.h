#pragma once
#include <stdint.h>

struct IAPGetIPodAuthSigPayload2p0 {
    uint8_t challenge[20];
    uint8_t retry;
} __attribute__((packed));

/*
struct IAPRetIPodAuthSigPayload {
    uint8_t sig[];
} __attribute__((packed));
*/

struct IAPAckIPodAuthSigPayload {
    uint8_t status;
} __attribute__((packed));
