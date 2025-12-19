#pragma once
#include <stdint.h>

/* [1] P.285 4.6.3 Command 0x04: NotifyUSBMode */

enum IAPUSBHostModeStatusCodes {
    Disabled   = 0x00,
    DeviceMode = 0x01,
    HostMode   = 0x02,
};

struct IAPNotifyUSBModePayload {
    uint8_t mode; /* IAPUSBHostModeStatusCodes */
};

struct IAPRetIPodUSBModePayload {
    uint8_t mode; /* IAPUSBHostModeStatusCodes */
};

struct IAPSetIPodUSBModePayload {
    uint8_t mode; /* IAPUSBHostModeStatusCodes */
};
