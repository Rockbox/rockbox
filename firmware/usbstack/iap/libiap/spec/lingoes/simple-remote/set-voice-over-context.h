#pragma once
#include <stdint.h>

enum IAPSetVoiceOverContextType {
    IAPSetVoiceOverContextType_None               = 0x00,
    IAPSetVoiceOverContextType_Header             = 0x01,
    IAPSetVoiceOverContextType_Link               = 0x02,
    IAPSetVoiceOverContextType_Form               = 0x03,
    IAPSetVoiceOverContextType_Cursor             = 0x04,
    IAPSetVoiceOverContextType_VerticalNavigation = 0x05,
    IAPSetVoiceOverContextType_ValueAdjustment    = 0x06,
    IAPSetVoiceOverContextType_ZoomAdjustment     = 0x07,
};

struct IAPSetVoiceOverContextPayload {
    uint8_t type; /* IAPSetVoiceOverContextType */
} __attribute__((packed));
