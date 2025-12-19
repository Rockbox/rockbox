#pragma once
#include <stdint.h>

/* [1] P.368 4.12.7 Command 0x04: AccessoryStateChangeEvent */

enum IAPAccessoryStateChangeEvents {
    IAPAccessoryStateChangeEvents_DisplaySwitchAwayFromIPodOut = 0x00,
    IAPAccessoryStateChangeEvents_DisplaySwitchBackToIPodOut   = 0x01,
    IAPAccessoryStateChangeEvents_AudioSwitchAwayFromIPodOut   = 0x02,
    IAPAccessoryStateChangeEvents_AudioSwitchBackToIPodOut     = 0x03,
    IAPAccessoryStateChangeEvents_EnterDaytimeMode             = 0x04,
    IAPAccessoryStateChangeEvents_EnterNighttimeMode           = 0x05,
};

struct AccessoryStateChangeEventPayload {
    uint8_t state_change; /* IAPAccessoryStateChangeEvents */
} __attribute__((packed));

