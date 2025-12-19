#pragma once
#include <stdint.h>

/* [1] P.366 4.12.4 Command 0x01: GetiPodOutOptions */

struct IAPGetIPodOutOptionsPayload {
    uint8_t report_current;
} __attribute__((packed));

enum IAPIPodOutOptionBits {
    IAPIPodOutOptionBits_DisplayAudio                   = 1 << 0,
    IAPIPodOutOptionBits_DisplayPhoneCall               = 1 << 1,
    IAPIPodOutOptionBits_DisplaySMS                     = 1 << 2,
    IAPIPodOutOptionBits_DisplayVoicemail               = 1 << 4,
    IAPIPodOutOptionBits_DisplayPushNotifications       = 1 << 5,
    IAPIPodOutOptionBits_DisplayClockAlarmNotifications = 1 << 6,
    IAPIPodOutOptionBits_DisplayTestPattern             = 1 << 8,
    IAPIPodOutOptionBits_ShowMinimalIPodOut             = 1 << 9,
    IAPIPodOutOptionBits_ShowFullIPodOud                = 1 << 10,
};

struct IAPRetIPodOutOptionsPayload {
    uint8_t  report_current;
    uint32_t options; /* IAPIPodOutOptionBits */
} __attribute__((packed));

struct IAPSetIPodOutOptionsPayload {
    uint32_t options; /* IAPIPodOutOptionBits */
} __attribute__((packed));
