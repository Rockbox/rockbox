#pragma once
#include "context.h"
#include "span.h"

#ifdef __cplusplus
extern "C" {
#endif

/* iap.c */
IAPBool        iap_init_ctx(struct IAPContext* ctx, struct IAPOpts opts, void* platform);
IAPBool        iap_deinit_ctx(struct IAPContext* ctx);
IAPBool        _iap_feed_packet(struct IAPContext* ctx, const uint8_t* data, size_t size);
struct IAPSpan _iap_get_buffer_for_send_payload(struct IAPContext* ctx);
int32_t        _iap_next_trans_id(struct IAPContext* ctx);
IAPBool        _iap_send_packet(struct IAPContext* ctx, uint8_t lingo, uint16_t command, int32_t trans_id, uint8_t* final_ptr);
/* must be called after iap_platform_on_acc_samprs_received */
IAPBool iap_select_sampr(struct IAPContext* ctx, uint32_t sampr);
IAPBool iap_control_response(struct IAPContext* ctx, struct IAPPlatformPendingControl pending, IAPBool result);

/* hid.c */
IAPBool iap_feed_hid_report(struct IAPContext* ctx, const uint8_t* data, size_t size);
IAPBool iap_notify_send_complete(struct IAPContext* ctx);
IAPBool _iap_send_hid_reports(struct IAPContext* ctx, size_t begin, size_t end); /* data is passed by ctx->send_buf */
IAPBool _iap_send_next_report(struct IAPContext* ctx);

/* fid-token-values.c */
int _iap_hanlde_set_fid_token_values(struct IAPSpan* request, struct IAPSpan* response);

/* notification.c */
void iap_notify_track_time_position(struct IAPContext* ctx, uint32_t pos_ms);
void iap_notify_track_playback_index(struct IAPContext* ctx, uint32_t index);
void iap_notify_track_caps(struct IAPContext* ctx, uint32_t caps);
void iap_notify_tracks_count(struct IAPContext* ctx, uint32_t count);
void iap_notify_play_status(struct IAPContext* ctx, uint8_t status /* IAPIPodStatePlayStatus */);
void iap_notify_volume(struct IAPContext* ctx, uint8_t volume, IAPBool muted);
void iap_notify_power_state(struct IAPContext* ctx, uint8_t state /* IAPIPodStatePowerState */, uint8_t battery_level);
void iap_notify_shuffle_state(struct IAPContext* ctx, uint8_t state /* IAPIPodStateShuffleSettingState */);
void iap_notify_repeat_state(struct IAPContext* ctx, uint8_t state /* IAPIPodStateRepeatSettingState */);
void iap_notify_time_setting(struct IAPContext* ctx, const struct IAPDateTime* time);
void iap_notify_hold_switch_state(struct IAPContext* ctx, uint8_t state);

IAPBool iap_periodic_tick(struct IAPContext* ctx); /* call every 100ms */

IAPBool _iap_flush_notification(struct IAPContext* ctx);

/* debug.c */
const char* _iap_lingo_str(uint8_t lingo);
const char* _iap_command_str(uint8_t lingo, uint16_t command);
IAPBool     _iap_span_is_str(const struct IAPSpan* span);
const char* _iap_span_as_str(const struct IAPSpan* span);
void        _iap_dump_packet(uint8_t lingo, uint16_t command, int32_t trans_id, struct IAPSpan span);

#ifdef __cplusplus
}
#endif
