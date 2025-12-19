#pragma once
#include <stdint.h>

/* [1] P.434 5.1.44 Command 0x0030: ReturnRepeat */

struct IAPReturnRepeatPayload {
    uint8_t mode; /* IAPIPodStateRepeaetSettingState */
};

struct IAPSetRepeatPayload {
    uint8_t mode;            /* IAPIPodStateRepeatSettingState */
    uint8_t restore_on_exit; /* optional */
};
