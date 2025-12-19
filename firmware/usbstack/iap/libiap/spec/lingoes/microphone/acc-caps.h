#pragma once
#include <stdint.h>

enum IAPAccCapsBits {
    IAPAccCapsBits_StereoInput           = 1 << 0,
    IAPAccCapsBits_StereoMonoInputSwitch = 1 << 1,
    IAPAccCapsBits_VariableRecordLevel   = 1 << 2,
    IAPAccCapsBits_RecordLevelLimit      = 1 << 3,
    IAPAccCapsBits_DuplexAudio           = 1 << 4,
};

struct IAPRetAccCapsPayload {
    uint8_t bits; /* IAPAccCapsBits */
} __attribute__((packed));
