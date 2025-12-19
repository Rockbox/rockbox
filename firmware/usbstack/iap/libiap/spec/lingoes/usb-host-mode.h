#pragma once
#include <stdint.h>

/* [1] P.283 Table 4-101 USB Host mode commands */
enum IAPUSBHostModeCommandID {
    IAPUSBHostModeCommandID_AccessoryAck   = 0x00, /* from acc, general/acc-ack.h */
    IAPUSBHostModeCommandID_NotifyUSBMode  = 0x04, /* from dev, usb-mode.h */
    IAPUSBHostModeCommandID_IPodAck        = 0x80, /* from dev, general/ipod-ack.h */
    IAPUSBHostModeCommandID_GetIPodUSBMode = 0x81, /* from acc, no payload */
    IAPUSBHostModeCommandID_RetIPodUSBMode = 0x82, /* from dev, usb-mode.h */
    IAPUSBHostModeCommandID_SetIPodUSBMode = 0x83, /* from acc, usb-mode.h */
};

#include "usb-host-mode/usb-mode.h"
