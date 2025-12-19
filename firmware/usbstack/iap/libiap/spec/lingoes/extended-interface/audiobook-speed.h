#pragma once
#include <stdint.h>

struct IAPRetAudiobookSpeedPayload {
    uint8_t speed; /* IAPIPodStateAudiobookSpeeed */
} __attribute__((packed));

struct IAPSetAudiobookSpeedPayload {
    uint8_t speed;           /* IAPIPodStateAudiobookSpeeed */
    uint8_t restore_on_exit; /* optional */
} __attribute__((packed));
