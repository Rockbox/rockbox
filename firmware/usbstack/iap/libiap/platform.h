#pragma once
#include <stddef.h>
#include <stdint.h>

#include "datetime.h"
#include "span.h"
#include "spec/iap.h"

#ifdef __cplusplus
extern "C" {
#endif

struct IAPContext;

/* iap_platform_malloc */
enum IAPPlatformMallocFlags {
    IAPPlatformMallocFlags_Uncached = 1 << 0,
};

/* iap_platform_get_play_status */
struct IAPPlatformPlayStatus {
    uint32_t track_total_ms;
    uint32_t track_pos_ms;
    uint32_t track_index;
    uint32_t track_count;
    uint32_t track_caps; /* IAPIPodStateTrackCapBits */
    uint8_t  state;      /* IAPIPodStatePlayStatus */
};

/* iap_platform_control */
enum IAPPlatformControl {
    IAPPlatformControl_TogglePlayPause,
    IAPPlatformControl_Play,
    IAPPlatformControl_Pause,
    IAPPlatformControl_Stop,
    IAPPlatformControl_Next,
    IAPPlatformControl_Prev,
    IAPPlatformControl_VolumeUp,
    IAPPlatformControl_VolumeDown,
    IAPPlatformControl_ToggleMute,
};

struct IAPPlatformPendingControl {
    uint16_t req_command;
    uint16_t ack_command;
    int32_t  trans_id;
    uint8_t  lingo;
};

/* iap_platform_get_volume */
struct IAPPlatformVolumeStatus {
    uint8_t volume;
    IAPBool muted;
};

/* iap_platform_get_power_status */
struct IAPPlatformPowerStatus {
    uint8_t state; /* IAPIPodStatePowerState */
    uint8_t battery_level;
};

/* iap_platform_get_indexed_track_info */
struct IAPPlatformTrackInfo {
    uint32_t*           total_ms;
    uint32_t*           caps; /* IAPIPodStateTrackCapBits */
    struct IAPDateTime* release_date;
    struct IAPSpan*     artist;
    struct IAPSpan*     composer;
    struct IAPSpan*     album;
    struct IAPSpan*     title;
};

/* iap_platform_open_artwork */
struct IAPPlatformArtwork {
    uint16_t  width;  /* user */
    uint16_t  height; /* user */
    IAPBool   color;  /* user */
    IAPBool   valid;  /* library */
    uintptr_t opaque; /* user */
};

/* library routines */
void* iap_platform_malloc(struct IAPContext* iap_ctx, size_t size, int flags);
void  iap_platform_free(struct IAPContext* iap_ctx, void* ptr);
int   iap_platform_send_hid_report(struct IAPContext* iap_ctx, const void* ptr, size_t size);

/* system info */
IAPBool iap_platform_get_ipod_serial_num(struct IAPContext* iap_ctx, struct IAPSpan* serial);

/* audio controls */
IAPBool iap_platform_get_play_status(struct IAPContext* iap_ctx, struct IAPPlatformPlayStatus* status);
void    iap_platform_control(struct IAPContext* iap_ctx, enum IAPPlatformControl control, struct IAPPlatformPendingControl pending);
IAPBool iap_platform_get_volume(struct IAPContext* iap_ctx, struct IAPPlatformVolumeStatus* status);
IAPBool iap_platform_get_power_status(struct IAPContext* iap_ctx, struct IAPPlatformPowerStatus* status);
IAPBool iap_platform_get_shuffle_setting(struct IAPContext* iap_ctx, uint8_t* status /* IAPIPodStateShuffleSettingState */);
IAPBool iap_platform_set_shuffle_setting(struct IAPContext* iap_ctx, uint8_t status /* IAPIPodStateShuffleSettingState */);
IAPBool iap_platform_get_repeat_setting(struct IAPContext* iap_ctx, uint8_t* status /* IAPIPodStateRepeatSettingState */);
IAPBool iap_platform_set_repeat_setting(struct IAPContext* iap_ctx, uint8_t status /* IAPIPodStateRepeatSettingState */);
IAPBool iap_platform_get_date_time(struct IAPContext* iap_ctx, struct IAPDateTime* time);
IAPBool iap_platform_get_backlight_level(struct IAPContext* iap_ctx, uint8_t* level);
IAPBool iap_platform_get_hold_switch_state(struct IAPContext* iap_ctx, IAPBool* state);
IAPBool iap_platform_get_indexed_track_info(struct IAPContext* iap_ctx, uint32_t index, struct IAPPlatformTrackInfo* info);
IAPBool iap_platform_set_playing_track(struct IAPContext* iap_ctx, uint32_t index);
IAPBool iap_platform_open_artwork(struct IAPContext* iap_ctx, uint32_t index, struct IAPPlatformArtwork* artwork);
IAPBool iap_platform_get_artwork_ptr(struct IAPContext* iap_ctx, struct IAPPlatformArtwork* artwork, struct IAPSpan* span);
IAPBool iap_platform_close_artwork(struct IAPContext* iap_ctx, struct IAPPlatformArtwork* artwork);

/* other callbacks */
IAPBool iap_platform_on_acc_samprs_received(struct IAPContext* iap_ctx, struct IAPSpan* samprs);

/* debugging */
void iap_platform_dump_hex(const void* ptr, size_t size);

#ifdef __cplusplus
}
#endif
