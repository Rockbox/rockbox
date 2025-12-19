#pragma once
#include <stdint.h>

/* [1] P.431 5.1.41 Command 0x002D: ReturnShuffle */

struct IAPReturnShufflePayload {
    uint8_t mode; /* IAPIPodStateShuffleSettingState */
};

struct IAPSetShufflePayload {
    uint8_t mode;            /* IAPIPodStateShuffleSettingState */
    uint8_t restore_on_exit; /* optional */
};
