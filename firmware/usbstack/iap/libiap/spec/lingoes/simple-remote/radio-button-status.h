#pragma once
#include <stdint.h>

enum IAPRadioButtonStatus {
    IAPRadioButtonStatus_Released = 0x00,
    IAPRadioButtonStatus_Pushed   = 0x02,
};

struct IAPRadioButtonStatusPayload {
    uint8_t status; /* IAPRadioButtonStatus */
} __attribute__((packed));
