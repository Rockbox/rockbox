#pragma once
#include <stdint.h>

enum IAPIPodOutButtonStatusSource {
    IAPIPodOutButtonStatusSource_CarCenterConsole = 0x00,
    IAPIPodOutButtonStatusSource_SteeringWheel    = 0x01,
    IAPIPodOutButtonStatusSource_CarDashboard     = 0x02,
};

enum IAPIPodOutButtonBits {
    /* bits1 */
    IAPIPodOutButtonBits_Select = 1 << 0,
    IAPIPodOutButtonBits_Left   = 1 << 1,
    IAPIPodOutButtonBits_Rigth  = 1 << 2,
    IAPIPodOutButtonBits_Up     = 1 << 3,
    IAPIPodOutButtonBits_Down   = 1 << 4,
    IAPIPodOutButtonBits_Menu   = 1 << 5,
};

struct IAPIPodOutButtonStatusPayload {
    uint8_t source; /* IAPIPodOutButtonStatusSource */
    uint8_t bits1;  /* IAPIPodOutButtonBits(bits1) */
    uint8_t bits2;  /* IAPIPodOutButtonBits(bits2), optional */
    uint8_t bits3;  /* IAPIPodOutButtonBits(bits3), optional */
    uint8_t bits4;  /* IAPIPodOutButtonBits(bits4), optional */
} __attribute__((packed));
