#pragma once
#include <stdint.h>

enum IAPGetVoiceOverParameterType {
    IAPGetVoiceOverParameterType_VoiceOverVolume  = 0x00,
    IAPGetVoiceOverParameterType_SpeakingRate     = 0x01,
    IAPGetVoiceOverParameterType_VoiceOverEnabled = 0x02,
};

struct IAPGetVoiceOverParameterPayload {
    uint8_t type; /* IAPGetVoiceOverParameterType */
} __attribute__((packed));

struct IAPRetVoiceOverParameterPayload {
    uint8_t type; /* IAPGetVoiceOverParameterType */
    uint8_t value;
} __attribute__((packed));

struct IAPSetVoiceOverParameterPayload {
    uint8_t type; /* IAPGetVoiceOverParameterType */
    uint8_t value;
} __attribute__((packed));


struct IAPVoiceOverParameterChangedPayload {
    uint8_t type; /* IAPGetVoiceOverParameterType */
    uint8_t value;
} __attribute__((packed));
