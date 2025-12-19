#pragma once
#include <stdint.h>

/* [1] P.152 Table 3-57 Apple device preference class and setting IDs */
enum IAPIPodPereferenceClassID {
    IAPIPodPereferenceClassID_VideoOut             = 0x00,
    IAPIPodPereferenceClassID_Screen               = 0x01,
    IAPIPodPereferenceClassID_VideoFormat          = 0x02,
    IAPIPodPereferenceClassID_LineOutUsage         = 0x03,
    IAPIPodPereferenceClassID_VideoOutConnection   = 0x08,
    IAPIPodPereferenceClassID_ClosedCaptioning     = 0x09,
    IAPIPodPereferenceClassID_VideoAspectRatio     = 0x0A,
    IAPIPodPereferenceClassID_Subtitle             = 0x0C,
    IAPIPodPereferenceClassID_VideoAltAudioChannel = 0x0D,
    IAPIPodPereferenceClassID_PauseOnPowerRemoval  = 0x0F,
    IAPIPodPereferenceClassID_VoiceOver            = 0x14,
    IAPIPodPereferenceClassID_AssistiveTouch       = 0x16,
};

enum IAPIPodPreferenceVideoOutSettingID {
    IAPIPodPreferenceVideoOutSettingID_Off = 0x00,
    IAPIPodPreferenceVideoOutSettingID_On  = 0x01,
};

enum IAPIPodPreferenceScreenSettingID {
    IAPIPodPreferenceScreenSettingID_FillScreen = 0x00,
    IAPIPodPreferenceScreenSettingID_FitToEdge  = 0x01,
};

enum IAPIPodPreferenceVideoFormatSettingID {
    IAPIPodPreferenceVideoFormatSettingID_NTSC = 0x00,
    IAPIPodPreferenceVideoFormatSettingID_PAL  = 0x01,
};

enum IAPIPodPreferenceLineOutUsageSettingID {
    IAPIPodPreferenceLineOutUsageSettingID_NotUsed = 0x00,
    IAPIPodPreferenceLineOutUsageSettingID_Used    = 0x01,
};

enum IAPIPodPreferenceVideoOutConnectionSettingID {
    IAPIPodPreferenceVideoOutConnectionSettingID_Composite = 0x01,
    IAPIPodPreferenceVideoOutConnectionSettingID_SVideo    = 0x02,
    IAPIPodPreferenceVideoOutConnectionSettingID_Component = 0x03,
};

enum IAPIPodPreferenceClosedCaptioningSettingID {
    IAPIPodPreferenceClosedCaptioningSettingID_Off = 0x00,
    IAPIPodPreferenceClosedCaptioningSettingID_On  = 0x01,
};

enum IAPIPodPreferenceVideoAspectRatioSettingID {
    IAPIPodPreferenceVideoAspectRatioSettingID_4x3  = 0x00,
    IAPIPodPreferenceVideoAspectRatioSettingID_16x9 = 0x01,
};

enum IAPIPodPreferenceSubtitleSettingID {
    IAPIPodPreferenceSubtitleSettingID_Off = 0x00,
    IAPIPodPreferenceSubtitleSettingID_On  = 0x01,

};

enum IAPIPodPreferenceVideoAltAudioChannelSettingID {
    IAPIPodPreferenceVideoAltAudioChannelSettingID_Off = 0x00,
    IAPIPodPreferenceVideoAltAudioChannelSettingID_On  = 0x01,
};

enum IAPIPodPreferencePauseOnPowerRemovalSettingID {
    IAPIPodPreferencePauseOnPowerRemovalSettingID_Off = 0x00,
    IAPIPodPreferencePauseOnPowerRemovalSettingID_On  = 0x01,
};

enum IAPIPodPreferenceVoiceOverSettingID {
    IAPIPodPreferenceVoiceOverSettingID_Off = 0x00,
    IAPIPodPreferenceVoiceOverSettingID_On  = 0x01,
};

enum IAPIPodPreferenceAssistiveTouchSettingID {
    IAPIPodPreferenceAssistiveTouchSettingID_Off = 0x00,
    IAPIPodPreferenceAssistiveTouchSettingID_On  = 0x01,
};

struct IAPGetIPodPreferencesPayload {
    uint8_t class_id; /* IAPIPodPereferenceClassID */
} __attribute__((packed));

struct IAPRetIPodPreferencesPayload {
    uint8_t class_id; /* IAPIPodPereferenceClassID */
    uint8_t setting_id;
} __attribute__((packed));

struct IAPSetIPodPreferencesPayload {
    uint8_t class_id; /* IAPIPodPereferenceClassID */
    uint8_t setting_id;
    uint8_t restore_on_exit;
} __attribute__((packed));
