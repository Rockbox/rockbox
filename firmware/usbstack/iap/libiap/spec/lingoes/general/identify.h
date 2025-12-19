#pragma once
#include <stdint.h>

/* [1] P.531 Table C-8 Identify packet */
struct IAPIdentifyPayload {
    uint8_t supported_lingo;
};

/* [1] P.531 Table C-9 Identify packet for high-power accessories */
struct IAPIdentifyHighPowerPayload {
    uint8_t supported_lingo; /* = IAPLingoID_AccessoryPower */
    uint8_t reserved;        /* = 0x00 */
    uint8_t num_valid_bits;  /* = 0x02 */
    uint8_t option_flags;    /* TODO: add bitfield definition */
} __attribute__((packed));
