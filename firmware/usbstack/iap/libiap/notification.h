#pragma once
#include <stdint.h>

#include "datetime.h"

struct _IAPNotifyState {
    uint32_t           track_time_position_ms;
    uint32_t           track_playback_index;
    uint32_t           track_caps; /* IAPIPodStateTrackCapBits */
    uint32_t           tracks_count;
    uint8_t            play_status; /* IAPIPodStatePlayStatus */
    uint8_t            mute_state;
    uint8_t            volume;
    uint8_t            power_state; /* IAPIPodStatePowerState */
    uint8_t            battery_level;
    uint8_t            shuffle_state; /* IAPIPodStateShuffleSettingState */
    uint8_t            repeat_state;  /* IAPIPodStateRepeatSettingState */
    struct IAPDateTime time_setting;
    uint8_t            hold_switch_state;
};
