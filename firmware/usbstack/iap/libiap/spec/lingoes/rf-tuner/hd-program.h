#pragma once
#include <stdint.h>

/* [1] P.317 4.7.40 Command 0x26: RetHDProgramServiceCount */

struct IAPRetHDProgramServiceCountPayload {
    uint8_t count;
    uint8_t analog_program_exists;
} __attribute__((packed));

struct IAPRetHDProgramServicePayload {
    uint8_t index;
} __attribute__((packed));

struct IAPSetHDProgramServicePayload {
    uint8_t index;
} __attribute__((packed));
