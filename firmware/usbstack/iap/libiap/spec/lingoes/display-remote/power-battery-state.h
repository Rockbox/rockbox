#pragma once
#include <stdint.h>

struct IAPRetPowerBatteryStatePayload {
    uint8_t power_state; /* IAPIPodStatePowerState */
    uint8_t battery_level;
} __attribute__((packed));
