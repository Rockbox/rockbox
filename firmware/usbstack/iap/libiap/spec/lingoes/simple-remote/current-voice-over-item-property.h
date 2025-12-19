#pragma once
#include <stdint.h>

enum IAPCurrentVoiceOverItemPropertyType {
    IAPCurrentVoiceOverItemPropertyType_Label,
    IAPCurrentVoiceOverItemPropertyType_Value,
    IAPCurrentVoiceOverItemPropertyType_Hint,
    IAPCurrentVoiceOverItemPropertyType_Frame,
    IAPCurrentVoiceOverItemPropertyType_Traits,
};

struct IAPGetCurrentVoiceOverItemPropertyPayload {
    uint8_t type; /* IAPCurrentVoiceOverItemPropertyType */
} __attribute__((packed));

struct IAPRetCurrentVoiceOverItemPropertyPayload {
    uint8_t type; /* IAPCurrentVoiceOverItemPropertyType */
    uint8_t value[];
} __attribute__((packed));

struct IAPRetCurrentVoiceOverItemPropertyLabelPayload {
    uint8_t  type; /* = IAPCurrentVoiceOverItemPropertyType_Label */
    uint16_t current_section_index;
    uint16_t last_section_index;
    char     text[];
} __attribute__((packed));

struct IAPRetCurrentVoiceOverItemPropertyVolumePayload {
    uint8_t  type; /* = IAPCurrentVoiceOverItemPropertyType_Volume */
    uint16_t current_section_index;
    uint16_t last_section_index;
    char     text[];
} __attribute__((packed));

struct IAPRetCurrentVoiceOverItemPropertyHintPayload {
    uint8_t  type; /* = IAPCurrentVoiceOverItemPropertyType_Hint */
    uint16_t current_section_index;
    uint16_t last_section_index;
    char     text[];
} __attribute__((packed));

struct IAPRetCurrentVoiceOverItemPropertyFramePayload {
    uint8_t  type; /* = IAPCurrentVoiceOverItemPropertyType_Frame */
    uint16_t top_x;
    uint16_t top_y;
    uint16_t bottom_x;
    uint16_t bottom_y;
} __attribute__((packed));

enum IAPRetCurrentVoiceOverItemPropertyTraits {
    IAPRetCurrentVoiceOverItemPropertyTraits_Button             = 0x0000,
    IAPRetCurrentVoiceOverItemPropertyTraits_Link               = 0x0001,
    IAPRetCurrentVoiceOverItemPropertyTraits_SearchField        = 0x0002,
    IAPRetCurrentVoiceOverItemPropertyTraits_Image              = 0x0003,
    IAPRetCurrentVoiceOverItemPropertyTraits_Selected           = 0x0004,
    IAPRetCurrentVoiceOverItemPropertyTraits_Sound              = 0x0005,
    IAPRetCurrentVoiceOverItemPropertyTraits_KeyboardKey        = 0x0006,
    IAPRetCurrentVoiceOverItemPropertyTraits_StaticText         = 0x0007,
    IAPRetCurrentVoiceOverItemPropertyTraits_SummaryElement     = 0x0008,
    IAPRetCurrentVoiceOverItemPropertyTraits_NotEnabled         = 0x0009,
    IAPRetCurrentVoiceOverItemPropertyTraits_UpdatesFrequently  = 0x000A,
    IAPRetCurrentVoiceOverItemPropertyTraits_StartsMediaSession = 0x000B,
    IAPRetCurrentVoiceOverItemPropertyTraits_Adjustable         = 0x000C,
    IAPRetCurrentVoiceOverItemPropertyTraits_BackButton         = 0x000D,
    IAPRetCurrentVoiceOverItemPropertyTraits_Map                = 0x000E,
    IAPRetCurrentVoiceOverItemPropertyTraits_DeleteKey          = 0x000F,
};

struct IAPRetCurrentVoiceOverItemPropertyTraitsPayload {
    uint8_t  type;     /* = IAPCurrentVoiceOverItemPropertyType_Traits */
    uint16_t traits[]; /* IAPRetCurrentVoiceOverItemPropertyTraits */
} __attribute__((packed));
