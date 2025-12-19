#pragma once
#include <stdint.h>

/* [1] P.127 3.3.4 Command 0x04: ReturnExtendedInterfaceMode */

struct IAPReturnExtendedInterfaceModePayload {
    uint8_t is_ext_mode;
} __attribute__((packed));
