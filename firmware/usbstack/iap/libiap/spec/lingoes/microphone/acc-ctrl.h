#pragma once
#include <stdint.h>

enum IAPAccCtrlType {
    IAPAccCtrlType_LineInput                    = 0x01,
    IAPAccCtrlType_RecordingLevelControl        = 0x02,
    IAPAccCtrlType_RecordingLevelLimiterControl = 0x03,
};

struct IAPGetAccCtrlPayload {
    uint8_t type; /* IAPAccCtrlType */
} __attribute__((packed));

struct IAPRetAccCtrlPayload {
    uint8_t type; /* IAPAccCtrlType */
    uint8_t value;
} __attribute__((packed));

struct IAPSetAccCtrlPayload {
    uint8_t type; /* IAPAccCtrlType */
    uint8_t value;
} __attribute__((packed));
