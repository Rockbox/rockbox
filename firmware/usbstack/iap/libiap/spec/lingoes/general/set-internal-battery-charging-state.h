#pragma once
#include <stdint.h>

enum IAPInternalBatteryChargingState {
    IAPInternalBatteryChargingState_DontCharge = 0x00,
    IAPInternalBatteryChargingState_MayCharge  = 0x01,
};

struct IAPInternalBatteryChargingStatePayload {
    uint8_t state; /* IAPInternalBatteryChargingState */
} __attribute__((packed));
