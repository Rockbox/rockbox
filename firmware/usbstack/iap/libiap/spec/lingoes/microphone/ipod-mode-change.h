#pragma once
#include <stdint.h>

/* [1] P.537 Table C-18 Mode values */
enum IAPIPodModeChangeModes {
    IAPIPodModeChangeModes_BeginAudioRecord   = 0x00,
    IAPIPodModeChangeModes_EndAudioRecord     = 0x01,
    IAPIPodModeChangeModes_BeginAudioPlayback = 0x02,
    IAPIPodModeChangeModes_EndAudioPlayback   = 0x03,
};

struct IAPIPodModeChangePayload {
    uint8_t mode; /* IAPIPodModeChangeModes */
} __attribute__((packed));
