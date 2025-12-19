#include <inttypes.h>
#include <string.h>

#include "constants.h"
#include "endian.h"
#include "iap.h"
#include "macros.h"
#include "pack-util.h"
#include "platform.h"
#include "span.h"
#include "spec/iap.h"
#include "unaligned.h"

enum TransIDSupport {
    TransIDUnknown,
    TransIDSupported,
    TransIDNotSupported,
};

static struct IAPContextButtons parse_context_button_bits(uint8_t bits[4], struct IAPContextButtons* current) {
    struct IAPContextButtons new;
    struct IAPContextButtons released;

#define process(field, Mask, bits)                                    \
    new.field      = !!(bits & IAPContextButtonStatusButtons_##Mask); \
    released.field = current->field && !new.field;

    process(play_pause, PlayPause, bits[0]);
    process(volume_up, VolumeUp, bits[0]);
    process(volume_down, VolumeDown, bits[0]);
    process(next_track, NextTrack, bits[0]);
    process(prev_track, PrevTrack, bits[0]);
    process(next_album, NextAlbum, bits[0]);
    process(prev_album, PrevAlbum, bits[0]);
    process(stop, Stop, bits[0]);
    process(play, PlayResume, bits[1]);
    process(pause, Pause, bits[1]);
    process(mute_toggle, MuteToggle, bits[1]);
    process(next_chapter, NextChapter, bits[1]);
    process(prev_chapter, PrevChapter, bits[1]);
    process(next_playlist, NextPlaylist, bits[1]);
    process(prev_playlist, PrevPlaylist, bits[1]);
    process(shuffle_advance, ShuffleSettingAdvance, bits[1]);
    process(repeat_advance, RepeatSettingAdvance, bits[2]);

#undef process

    *current = new;
    return released;
}

IAPBool iap_init_ctx(struct IAPContext* ctx, struct IAPOpts opts, void* platform) {
    const uint16_t max_input_hid_desc_size = opts.usb_highspeed ? 0x02FF : 0x3F;

    memset(ctx, 0, sizeof(*ctx));
    ctx->opts     = opts;
    ctx->platform = platform;

    ctx->hid_recv_buf = iap_platform_malloc(ctx, HID_BUFFER_SIZE, 0);
    check_ret(ctx->hid_recv_buf != NULL, iap_false);
    ctx->send_buf = iap_platform_malloc(ctx, SEND_BUFFER_SIZE, 0);
    check_ret(ctx->send_buf != NULL, iap_false);
    ctx->handling_trans_id    = -1;
    ctx->trans_id_support     = TransIDUnknown;
    ctx->hid_send_staging_buf = iap_platform_malloc(ctx, max_input_hid_desc_size + 1 /* report id */, IAPPlatformMallocFlags_Uncached);
    check_ret(ctx->hid_send_staging_buf != NULL, iap_false);
    ctx->phase = IAPPhase_Connected;
    return iap_true;
}

IAPBool iap_deinit_ctx(struct IAPContext* ctx) {
    if(ctx->artwork.valid) {
        iap_platform_close_artwork(ctx, &ctx->artwork);
    }
    iap_platform_free(ctx, ctx->hid_send_staging_buf);
    iap_platform_free(ctx, ctx->hid_recv_buf);
    iap_platform_free(ctx, ctx->send_buf);
    return iap_true;
}

#define read_request(Type)                                                      \
    const struct Type* request = iap_span_read(request_span, sizeof(*request)); \
    check_ret(request != NULL, -IAPAckStatus_EBadParameter);

#define alloc_response_extra(Type, extra)                                             \
    struct Type* response = iap_span_alloc(response_span, sizeof(*response) + extra); \
    check_ret(response != NULL, -IAPAckStatus_EOutOfResource);

#define alloc_response(Type) alloc_response_extra(Type, 0)

static uint32_t play_stage_change_notification_set_mask_to_type_mask(uint32_t mask) {
    uint32_t ret = 0;
    if(mask & IAPStatusChangeNotificationBits_Basic) {
        ret |= 1 << IAPStatusChangeNotificationType_PlaybackStopped |
               1 << IAPStatusChangeNotificationType_PlaybackFEWSeekStop |
               1 << IAPStatusChangeNotificationType_PlaybackREWSeekStop;
    }
    if(mask & IAPStatusChangeNotificationBits_Extended) {
        ret |= 1 << IAPStatusChangeNotificationType_PlaybackStatusExtended;
    }
    if(mask & IAPStatusChangeNotificationBits_TrackIndex) {
        ret |= 1 << IAPStatusChangeNotificationType_TrackIndex;
    }
    if(mask & IAPStatusChangeNotificationBits_TrackTimeOffsetMSec) {
        ret |= 1 << IAPStatusChangeNotificationType_TrackTimeOffsetMSec;
    }
    if(mask & IAPStatusChangeNotificationBits_TrackTimeOffsetSec) {
        ret |= 1 << IAPStatusChangeNotificationType_TrackTimeOffsetSec;
    }
    if(mask & IAPStatusChangeNotificationBits_PlaybackEngineContentsChanged) {
        ret |= 1 << IAPStatusChangeNotificationType_PlaybackEngineContentsChanged;
    }
    return ret;
}

static IAPBool send_artwork_chunk_cb(struct IAPContext* ctx) {
    struct IAPSpan request = _iap_get_buffer_for_send_payload(ctx);
    if(ctx->artwork_chunk_index == 0) {
        struct IAPRetTrackArtworkDataFirstPayload* payload = iap_span_alloc(&request, sizeof(*payload));

        payload->index                = swap_16(ctx->artwork_chunk_index);
        payload->pixel_format         = ctx->artwork.color ? IAPArtworkPixelFormats_RGB565LE : IAPArtworkPixelFormats_Mono;
        payload->pixel_width          = swap_16(ctx->artwork.width);
        payload->pixel_height         = swap_16(ctx->artwork.height);
        payload->inset_top_left_x     = 0;
        payload->inset_top_left_y     = 0;
        payload->inset_bottom_right_x = payload->pixel_width;
        payload->inset_bottom_right_y = payload->pixel_height;
        payload->stride               = swap_32(ctx->artwork.width * 2); /* TODO: support stride */
    } else {
        struct IAPRetTrackArtworkDataSubsequenctPayload* payload = iap_span_alloc(&request, sizeof(*payload));

        payload->index = swap_16(ctx->artwork_chunk_index);
    }
    struct IAPSpan artwork;
    size_t         copy_size = 0;
    if(!ctx->opts.artwork_single_report || ctx->artwork_chunk_index != 0) {
        check_ret(iap_platform_get_artwork_ptr(ctx, &ctx->artwork, &artwork), iap_false);
        check_ret(iap_span_read(&artwork, ctx->artwork_cursor) != NULL, iap_false); /* skip already read chunk */
        copy_size = min((ctx->opts.artwork_single_report ? 48 : request.size), artwork.size);
        memcpy(iap_span_alloc(&request, copy_size), iap_span_read(&artwork, copy_size), copy_size);
    }
    check_ret(_iap_send_packet(ctx, ctx->artwork_data_lingo, ctx->artwork_data_command, ctx->artwork_trans_id, request.ptr), iap_false);
    if(artwork.size > 0) {
        /* more to send, ask to call again */
        ctx->artwork_cursor += copy_size;
        ctx->artwork_chunk_index += 1;
        ctx->on_send_complete = send_artwork_chunk_cb;
        print("track artwork left %zu bytes", artwork.size);
    } else {
        /* finished, free artwork */
        check_ret(iap_platform_close_artwork(ctx, &ctx->artwork), iap_false);
        ctx->artwork.valid = iap_false;
        print("track artwork done");
    }
    return iap_true;
}

static int32_t start_artwork_data(struct IAPContext* ctx, struct IAPSpan* request_span, IAPBool ext) {
    read_request(IAPGetTrackArtworkDataPayload);
    check_ret(request->format_id == 0, -IAPAckStatus_EBadParameter);
    check_ret(request->offset_ms == 0, -IAPAckStatus_EBadParameter);
    check_ret(!ctx->artwork.valid, -IAPAckStatus_EBadParameter);

    check_ret(iap_platform_open_artwork(ctx, swap_32(request->track_index), &ctx->artwork), -IAPAckStatus_EBadParameter);
    ctx->artwork.valid       = iap_true;
    ctx->artwork_cursor      = 0;
    ctx->artwork_chunk_index = 0;
    ctx->artwork_trans_id    = ctx->handling_trans_id;
    if(ext) {
        ctx->artwork_data_lingo   = IAPLingoID_ExtendedInterface;
        ctx->artwork_data_command = IAPExtendedInterfaceCommandID_RetTrackArtworkData;
    } else {
        ctx->artwork_data_lingo   = IAPLingoID_DisplayRemote;
        ctx->artwork_data_command = IAPDisplayRemoteCommandID_RetTrackArtworkData;
    }
    check_ret(send_artwork_chunk_cb(ctx), -IAPAckStatus_ECommandFailed);
    return 0;
}

static int32_t ipod_ack(uint16_t command, enum IAPAckStatus status, struct IAPSpan* response_span, uint16_t ret) {
    alloc_response(IAPIPodAckPayload);
    response->status = status;
    response->id     = command;
    return ret;
}

static int32_t ipod_ack_ext(uint16_t command, enum IAPAckStatus status, struct IAPSpan* response_span) {
    alloc_response(IAPExtendedIPodAckPayload);
    response->id     = swap_16(command);
    response->status = status;
    return IAPExtendedInterfaceCommandID_IPodAck;
}

static int32_t handle_command(struct IAPContext* ctx, uint8_t lingo, uint16_t command, struct IAPSpan* request_span, struct IAPSpan* response_span) {
    switch(lingo) {
    case IAPLingoID_General:
        switch(command) {
        case IAPGeneralCommandID_RequestExtendedInterfaceMode: {
            alloc_response(IAPReturnExtendedInterfaceModePayload);
            response->is_ext_mode = 1;
            return IAPGeneralCommandID_ReturnExtendedInterfaceMode;
        } break;
        case IAPGeneralCommandID_RequestIPodSoftwareVersion: {
            alloc_response(IAPReturnIPodSoftwareVersionPayload);
            response->major    = 18;
            response->minor    = 7;
            response->revision = 2;
            return IAPGeneralCommandID_ReturnIPodSoftwareVersion;
        } break;
        case IAPGeneralCommandID_RequestIPodSerialNum: {
            check_ret(iap_platform_get_ipod_serial_num(ctx, response_span), -IAPAckStatus_ECommandFailed);
            return IAPGeneralCommandID_ReturnIPodSerialNum;
        } break;
        case IAPGeneralCommandID_RequestIPodModelNum: {
            static const char* model_num = "MTAY2J/A";
            check_ret(iap_span_append(response_span, model_num, strlen(model_num) + 1), -IAPAckStatus_EOutOfResource);
            return IAPGeneralCommandID_ReturnIPodModelNum;
        } break;
        case IAPGeneralCommandID_RequestTransportMaxPayloadSize: {
            alloc_response(IAPReturnTransportMaxPayloadSizePayload);
            response->max_payload_size = swap_16(HID_BUFFER_SIZE - 1 /*sync*/ - 1 /*sof*/ - 3 /*length*/ - 1 /*checksum*/);
            return IAPGeneralCommandID_ReturnTransportMaxPayloadSize;
        } break;
        case IAPGeneralCommandID_RequestLingoProtocolVersion: {
            static const struct {
                uint8_t major;
                uint8_t minor;
            } table[] = {
                [IAPLingoID_General]            = {1, 9},
                [IAPLingoID_Microphone]         = {1, 1},
                [IAPLingoID_SimpleRemote]       = {1, 4},
                [IAPLingoID_DisplayRemote]      = {1, 5},
                [IAPLingoID_ExtendedInterface]  = {1, 14},
                [IAPLingoID_AccessoryPower]     = {1, 1},
                [IAPLingoID_USBHostMode]        = {1, 0},
                [IAPLingoID_RFTuner]            = {1, 1},
                [IAPLingoID_AccessoryEqualizer] = {1, 0},
                [IAPLingoID_Sports]             = {1, 1},
                [IAPLingoID_DigitalAudio]       = {1, 3},
                [IAPLingoID_Storage]            = {1, 2},
                [IAPLingoID_IPodOut]            = {1, 0},
                [IAPLingoID_Location]           = {1, 0},
            };

            read_request(IAPRequestLingoProtocolVersionPayload);
            check_ret(request->lingo < array_size(table), -IAPAckStatus_EBadParameter);

            alloc_response(IAPReturnLingoProtocolVersionPayload);
            response->lingo = request->lingo;
            response->major = table[request->lingo].major;
            response->minor = table[request->lingo].minor;
            return IAPGeneralCommandID_ReturnLingoProtocolVersion;
        } break;
        case IAPGeneralCommandID_GetIPodOptions: {
            alloc_response(IAPRetIPodOptionsPayload);
            response->state = 0;
            return IAPGeneralCommandID_RetIPodOptions;
        } break;
        case IAPGeneralCommandID_GetIPodPreferences: {
            read_request(IAPGetIPodPreferencesPayload);
            alloc_response(IAPRetIPodPreferencesPayload);
            response->class_id   = request->class_id;
            response->setting_id = 0; /* TODO: return actual value */
            return IAPGeneralCommandID_RetIPodPreferences;
        } break;
        case IAPGeneralCommandID_SetIPodPreferences: {
            read_request(IAPSetIPodPreferencesPayload);
            /* TODO: handle preferences */
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
        } break;
        case IAPGeneralCommandID_SetUIMode: {
            read_request(IAPSetUIModePayload);
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
        } break;
        case IAPGeneralCommandID_GetIPodOptionsForLingo: {
            read_request(IAPGetIPodOptionsForLingoPayload);
            alloc_response(IAPRetIPodOptionsForLingoPayload);
            response->lingo_id = request->lingo_id;
            switch(request->lingo_id) {
            case IAPLingoID_SimpleRemote:
                response->bits = swap_64(IAPRetIPodOptionsForLingoSimpleRemoteBits_ContextSpecificControls);
                break;
            case IAPLingoID_General:
            case IAPLingoID_DisplayRemote:
            case IAPLingoID_ExtendedInterface:
            case IAPLingoID_DigitalAudio:
            case IAPLingoID_Storage: /* TODO: this is not supported */
                response->bits = 0;
                break;
            case IAPLingoID_USBHostMode:
            case IAPLingoID_RFTuner:
            case IAPLingoID_Sports:
            case IAPLingoID_IPodOut:
            case IAPLingoID_Location: { /* not supported */
                return -IAPAckStatus_EBadParameter;
            }
            }
            return IAPGeneralCommandID_RetIPodOptionsForLingo;
        } break;
        case IAPGeneralCommandID_GetSupportedEventNotification: {
            alloc_response(IAPRetSupportedEventNotificationPayload);
            response->mask = swap_64(IAPSetEventNotificationEvents_FlowControl);
            return IAPGeneralCommandID_RetSupportedEventNotification;
        } break;
        case IAPGeneralCommandID_SetAvailableCurrent: {
            read_request(IAPSetAvailableCurrentPayload);
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
        } break;
        case IAPGeneralCommandID_SetEventNotification: {
            read_request(IAPSetEventNotificationPayload);
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
        } break;
        }
        break;
    case IAPLingoID_SimpleRemote:
        switch(command) {
        case IAPSimpleRemoteCommandID_ContextButtonStatus: {
            response_span->ptr = NULL;

            uint8_t bits[4] = {0};
            for(int i = 0; i < 4 && request_span->size > 0; i += 1) {
                iap_span_read_8(request_span, &bits[i]);
            }
            const struct IAPContextButtons         released = parse_context_button_bits(bits, &ctx->context_button_state);
            const struct IAPPlatformPendingControl pending  = {
                 .req_command = command,
                 .ack_command = -1,
                 .trans_id    = ctx->handling_trans_id,
                 .lingo       = lingo,
            };
            const struct {
                IAPBool                 released;
                enum IAPPlatformControl control;
            } table[] = {
                {released.play_pause, IAPPlatformControl_TogglePlayPause},
                {released.volume_up, IAPPlatformControl_VolumeUp},
                {released.volume_down, IAPPlatformControl_VolumeDown},
                {released.next_track, IAPPlatformControl_Next},
                {released.prev_track, IAPPlatformControl_Prev},
                {released.next_album, IAPPlatformControl_Next},
                {released.prev_album, IAPPlatformControl_Prev},
                {released.stop, IAPPlatformControl_Stop},
                {released.play, IAPPlatformControl_Play},
                {released.pause, IAPPlatformControl_Pause},
                {released.mute_toggle, IAPPlatformControl_ToggleMute},
                {released.next_chapter, IAPPlatformControl_Next},
                {released.prev_chapter, IAPPlatformControl_Prev},
                {released.next_playlist, IAPPlatformControl_Next},
                {released.prev_playlist, IAPPlatformControl_Prev},
            };
            for(size_t i = 0; i < array_size(table); i += 1) {
                if(table[i].released) {
                    iap_platform_control(ctx, table[i].control, pending);
                }
            }
            if(released.shuffle_advance) {
                uint8_t current;
                check_act(iap_platform_get_shuffle_setting(ctx, &current), return 0);
                current = (current + 1) % IAPIPodStateShuffleSettingState_Albums;
                check_act(iap_platform_set_shuffle_setting(ctx, current), return 0);
            }
            if(released.repeat_advance) {
                uint8_t current;
                check_act(iap_platform_get_repeat_setting(ctx, &current), return 0);
                current = (current + 1) % IAPIPodStateRepeatSettingState_All;
                check_act(iap_platform_set_repeat_setting(ctx, current), return 0);
            }
            return 0;
        } break;
        }
        break;
    case IAPLingoID_DisplayRemote:
        switch(command) {
        case IAPDisplayRemoteCommandID_SetCurrentEQProfileIndex: {
            read_request(IAPSetCurrentEQProfileIndexPayload);
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPDisplayRemoteCommandID_IPodAck);
        } break;
        case IAPDisplayRemoteCommandID_SetRemoteEventNotification: {
            read_request(IAPSetRemoteEventNotificationPayload);
            ctx->notifications_3         = 0;
            ctx->enabled_notifications_3 = swap_32(request->mask);
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPDisplayRemoteCommandID_IPodAck);
        } break;
        case IAPDisplayRemoteCommandID_GetIPodStateInfo: {
            read_request(IAPGetIPodStateInfoPayload);
            check_ret(response_span->size >= sizeof(struct IAPIPodStatePayload), -IAPAckStatus_EOutOfResource);
            ((struct IAPIPodStatePayload*)response_span->ptr)->type = request->type;
            switch(request->type) {
            case IAPIPodStateType_TrackTimePositionMSec: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateTrackTimePositionMSecPayload);
                response->position_ms = swap_32(status.track_pos_ms);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_TrackPlaybackIndex: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateTrackPlaybackIndexPayload);
                response->index = swap_32(status.track_index);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_ChapterIndex: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateChapterIndexPayload);
                response->index = swap_32(status.track_index);
                /* no chapters */
                response->chapter_count = 0;
                response->chapter_index = -1;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_PlayStatus: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStatePlayStatusPayload);
                response->status = status.state; /* TODO: convert enum */
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_Volume: {
                struct IAPPlatformVolumeStatus status;
                check_ret(iap_platform_get_volume(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateVolumePayload);
                response->mute_state = status.muted;
                response->ui_volume  = status.volume;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_Power: {
                struct IAPPlatformPowerStatus status;
                check_ret(iap_platform_get_power_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStatePowerPayload);
                response->power_state   = status.state; /* TODO: convert enum */
                response->battery_level = status.battery_level;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_EQSetting: {
                alloc_response(IAPIPodStateEQSettingPayload);
                /* no eq setting support yet */
                response->eq_index = 0;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_ShuffleSetting: {
                alloc_response(IAPIPodStateShuffleSettingPayload);
                check_ret(iap_platform_get_shuffle_setting(ctx, &response->shuffle_state), -IAPAckStatus_ECommandFailed);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_RepeatSetting: {
                alloc_response(IAPIPodStateRepeatSettingPayload);
                check_ret(iap_platform_get_repeat_setting(ctx, &response->repeat_state), -IAPAckStatus_ECommandFailed);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_DateTimeSetting: {
                struct IAPDateTime time;
                check_ret(iap_platform_get_date_time(ctx, &time), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateDateTimeSettingPayload);
                response->year   = swap_16(time.year);
                response->month  = time.month;
                response->day    = time.day;
                response->hour   = time.hour;
                response->minute = time.minute;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_AlarmSetting: {
                alloc_response(IAPIPodStateAlarmSettingPayload);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_BacklightLevel: {
                alloc_response(IAPIPodStateBacklightLevelPayload);
                check_ret(iap_platform_get_backlight_level(ctx, &response->level), -IAPAckStatus_ECommandFailed);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_HoldSwitchState: {
                IAPBool state;
                check_ret(iap_platform_get_hold_switch_state(ctx, &state), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateHoldSwitchStatePayload);
                response->state = state;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_SoundCheckState: {
                alloc_response(IAPIPodStateSoundCheckStatePayload);
                response->state = 0; /* no sound check */
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_AudiobookSpeeed: {
                alloc_response(IAPIPodStateAudiobookSpeeedPayload);
                response->speed = IAPIPodStateAudiobookSpeeed_Normal;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_TrackTimePositionSec: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateTrackTimePositionSecPayload);
                response->position_s = swap_16(status.track_pos_ms / 1000);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_AbsoluteVolume: {
                struct IAPPlatformVolumeStatus status;
                check_ret(iap_platform_get_volume(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateAbsoluteVolumePayload);
                response->mute_state      = status.muted;
                response->ui_volume       = status.volume;
                response->absolute_volume = status.volume;
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_TrackCaps: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                alloc_response(IAPIPodStateTrackCapsPayload);
                response->caps = swap_32(status.track_caps);
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            case IAPIPodStateType_PlaybackEngineContents: {
                alloc_response(IAPIPodStatePlaybackEngineContentsPayload);
                response->count = 0; /* TODO: shoud be supported? */
                return IAPDisplayRemoteCommandID_RetIPodStateInfo;
            } break;
            default:
                warn("invalid request type 0x%02" PRIX8, request->type);
                return -IAPAckStatus_EBadParameter;
            }
        } break;
        case IAPDisplayRemoteCommandID_GetPlayStatus: {
            struct IAPPlatformPlayStatus status;
            check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
            alloc_response(IAPRetPlayStatusPayload);
            response->state          = status.state; /* TODO: convert enum */
            response->track_index    = swap_32(status.track_index);
            response->track_pos_ms   = swap_32(status.track_pos_ms);
            response->track_total_ms = swap_32(status.track_total_ms);
            return IAPDisplayRemoteCommandID_RetPlayStatus;
        } break;
        case IAPDisplayRemoteCommandID_GetIndexedPlayingTrackInfo: {
            read_request(IAPGetIndexedPlayingTrackInfoPayload);
            check_ret(response_span->size > sizeof(struct IAPRetIndexedPlayingTrackInfoPayload), -IAPAckStatus_EBadParameter);
            ((struct IAPRetIndexedPlayingTrackInfoPayload*)response_span->ptr)->type = request->type;
            switch(request->type) {
            case IAPIndexedPlayingTrackInfoType_TrackCapsInfo: {
                uint32_t                    length;
                uint32_t                    caps;
                struct IAPPlatformTrackInfo info = {.total_ms = &length, .caps = &caps};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                alloc_response(IAPRetIndexedPlayingTrackInfoTrackCapsInfoPayload);
                response->track_caps     = swap_32(caps);
                response->track_total_ms = swap_32(length);
                response->chapter_count  = 0;
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_ChapterTimeName: {
                return -IAPAckStatus_EBadParameter;
            } break;
            case IAPIndexedPlayingTrackInfoType_ArtistName: {
                alloc_response(IAPRetIndexedPlayingTrackInfoArtistNamePayload);
                struct IAPPlatformTrackInfo info = {.artist = response_span};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_AlbumName: {
                alloc_response(IAPRetIndexedPlayingTrackInfoAlbumNamePayload);
                struct IAPPlatformTrackInfo info = {.album = response_span};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_GenreName: {
                alloc_response_extra(IAPRetIndexedPlayingTrackInfoGenreNamePayload, 1);
                response->name[0] = '\0';
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_TrackTitle: {
                alloc_response(IAPRetIndexedPlayingTrackInfoTrackTitlePayload);
                struct IAPPlatformTrackInfo info = {.title = response_span};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_ComposerName: {
                alloc_response(IAPRetIndexedPlayingTrackInfoComposerNamePayload);
                struct IAPPlatformTrackInfo info = {.composer = response_span};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_Lyrics: {
                alloc_response_extra(IAPRetIndexedPlayingTrackInfoLyricsPayload, 1);
                response->info_bits = 0;
                response->index     = 0;
                response->lyrics[0] = '\0';
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            case IAPIndexedPlayingTrackInfoType_ArtworkCount: {
                alloc_response_extra(IAPRetIndexedPlayingTrackInfoArtworkCountPayload, sizeof(struct IAPArtworkCount));
                response->data[0].format = 0;
                response->data[0].count  = swap_16(1);
                return IAPDisplayRemoteCommandID_RetIndexedPlayingTrackInfo;
            } break;
            }
        } break;
        case IAPDisplayRemoteCommandID_GetArtworkFormats: {
            alloc_response_extra(IAPArtworkFormat, sizeof(struct IAPArtworkFormat));
            response->format_id    = 0;
            response->pixel_format = IAP_COLOR_ARTWORK ? IAPArtworkPixelFormats_RGB565LE : IAPArtworkPixelFormats_Mono;
            response->image_width  = swap_16(IAP_ARTWORK_WIDTH);
            response->image_height = swap_16(IAP_ARTWORK_HEIGHT);
            return IAPDisplayRemoteCommandID_RetArtworkFormats;
        } break;
        case IAPDisplayRemoteCommandID_GetNumPlayingTracks: {
            struct IAPPlatformPlayStatus status;
            check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
            alloc_response(IAPRetNumPlayingTracksPayload);
            response->num_playing_tracks = swap_32(status.track_count);
            return IAPDisplayRemoteCommandID_RetNumPlayingTracks;
        } break;
        case IAPDisplayRemoteCommandID_GetTrackArtworkData: {
            const int32_t ret = start_artwork_data(ctx, request_span, iap_false);
            check_ret(ret == 0, ret);
            /* responded in send_artwork_chunk_cb, no need to do it here */
            response_span->ptr = NULL;
            return 0;
        } break;
        case IAPDisplayRemoteCommandID_GetPowerBatteryState: {
            struct IAPPlatformPowerStatus status;
            check_ret(iap_platform_get_power_status(ctx, &status), -IAPAckStatus_ECommandFailed);
            alloc_response(IAPRetPowerBatteryStatePayload);
            response->power_state   = status.state; /* TODO: convert enum */
            response->battery_level = status.battery_level;
            return IAPDisplayRemoteCommandID_RetPowerBatteryState;
        } break;
        case IAPDisplayRemoteCommandID_GetTrackArtworkTimes: {
            read_request(IAPGetTrackArtworkTimesPayload);
            const uint16_t count = swap_16(request->artwork_count);
            check_ret(count == 0 || count == 1, -IAPAckStatus_ECommandFailed, "not implemented");

            void* payload = iap_span_alloc(response_span, sizeof(uint32_t) * count);
            check_ret(payload != NULL, iap_false);
            memset(payload, 0, sizeof(uint32_t) * count);
            return IAPDisplayRemoteCommandID_RetTrackArtworkTimes;
        } break;
        }
        break;
    case IAPLingoID_ExtendedInterface:
        switch(command) {
        case IAPExtendedInterfaceCommandID_GetCurrentPlayingTrackChapterInfo: {
            alloc_response(IAPReturnCurrentPlayingTrackChapterInfoPayload);
            /* no chapters */
            response->count = 0;
            response->index = -1;
            return IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackChapterInfo;
        } break;
        case IAPExtendedInterfaceCommandID_GetAudiobookSpeed: {
            alloc_response(IAPRetAudiobookSpeedPayload);
            response->speed = IAPIPodStateAudiobookSpeeed_Normal;
            return IAPExtendedInterfaceCommandID_RetAudiobookSpeed;
        } break;
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackInfo: {
            read_request(IAPExtendedGetIndexedPlayingTrackInfoPayload);
            check_ret(response_span->size > sizeof(struct IAPExtendedRetIndexedPlayingTrackInfoPayload), -IAPAckStatus_EBadParameter);
            ((struct IAPExtendedRetIndexedPlayingTrackInfoPayload*)response_span->ptr)->type = request->type;
            switch(request->type) {
            case IAPExtendedIndexedPlayingTrackInfoType_TrackCapsInfo: {
                uint32_t                    length;
                uint32_t                    caps;
                struct IAPPlatformTrackInfo info = {.total_ms = &length, .caps = &caps};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                alloc_response(IAPExtendedRetIndexedPlayingTrackInfoTrackCapsInfoPayload);
                response->track_caps     = swap_32(caps);
                response->track_total_ms = swap_32(length);
                response->chapter_count  = 0;
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_PodcastName: {
                alloc_response_extra(IAPExtendedRetIndexedPlayingTrackInfoPodcastNamePayload, 1);
                response->name[0] = '\0';
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackReleaseDate: {
                struct IAPDateTime          time;
                struct IAPPlatformTrackInfo info = {.release_date = &time};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                alloc_response(IAPExtendedRetIndexedPlayingTrackInfoTrackReleaseDatePayload);
                response->seconds = time.seconds;
                response->minutes = time.minute;
                response->hours   = time.hour;
                response->day     = time.day;
                response->month   = time.month;
                response->year    = swap_16(time.year);
                response->weekday = 0; /* TODO: set weekday? */
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackDescription: {
                alloc_response_extra(IAPExtendedRetIndexedPlayingTrackInfoTrackDescriptionPayload, 1);
                response->info_bits      = 0;
                response->index          = 0;
                response->description[0] = '\0';
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackSongLyrics: {
                alloc_response_extra(IAPExtendedRetIndexedPlayingTrackInfoTrackSongLyricsPayload, 1);
                response->info_bits = 0;
                response->index     = 0;
                response->lyrics[0] = '\0';
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackGenre: {
                alloc_response_extra(IAPExtendedRetIndexedPlayingTrackInfoTrackGenrePayload, 1);
                response->genre[0] = '\0';
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackComposer: {
                alloc_response(IAPExtendedRetIndexedPlayingTrackInfoTrackComposerPayload);
                struct IAPPlatformTrackInfo info = {.composer = response_span};
                check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->track_index), &info), -IAPAckStatus_EBadParameter);
                return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackInfo;
            } break;
            case IAPExtendedIndexedPlayingTrackInfoType_TrackArtworkCount: {
                // IAPArtworkPixelFormats_RGB565LE;
                // struct IAPExtendedRetIndexedPlayingTrackInfoTrackArtworkCountPayload payload = {
                //     .};
                warn("artwork not implemented");
                return -IAPAckStatus_ECommandFailed;
            } break;
            default:
                warn("invalid request type 0x%02" PRIX8, request->type);
                return -IAPAckStatus_EBadParameter;
            }
        } break;
        case IAPExtendedInterfaceCommandID_GetArtworkFormats: {
            /* same as DisplayRemote::GetArtworkFormats */
            const int32_t ret = handle_command(ctx, IAPLingoID_DisplayRemote, IAPDisplayRemoteCommandID_GetArtworkFormats, request_span, response_span);
            check_ret(ret == IAPDisplayRemoteCommandID_RetArtworkFormats, ret);
            return IAPExtendedInterfaceCommandID_RetArtworkFormats;
        } break;
        case IAPExtendedInterfaceCommandID_GetTrackArtworkData: {
            const int32_t ret = start_artwork_data(ctx, request_span, iap_true);
            check_ret(ret == 0, ret);
            /* responded in send_artwork_chunk_cb, no need to do it here */
            response_span->ptr = NULL;
            return 0;
        } break;
        case IAPExtendedInterfaceCommandID_ResetDBSelection: {
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_GetNumberCategorizedDBRecords: {
            read_request(IAPGetNumberCategorizedDBRecordsPayload);
            uint32_t count;
            switch(request->type) {
            case IAPDatabaseType_Playlist: {
                /* TODO: implement platform callback */
                count = 99;
            } break;
            case IAPDatabaseType_Track: {
                struct IAPPlatformPlayStatus status;
                check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
                /* track_count is invalid while stopped.
                 * return non-zero dummy value in this case, because reporting zero tracks
                 * may cause empty library error. */
                /* TODO: maybe add dedicated platform callback? */
                count = status.state == IAPIPodStatePlayStatus_PlaybackStopped ? 99 : status.track_count;
            } break;
            default: {
                warn("unsupported type 0x%02" PRIX8, request->type);
                count = 0;
            } break;
            }

            alloc_response(IAPReturnNumberCategorizedDBRecordsPayload);
            response->count = swap_32(count);
            return IAPExtendedInterfaceCommandID_ReturnNumberCategorizedDBRecords;
        } break;
        case IAPExtendedInterfaceCommandID_GetPlayStatus: {
            struct IAPPlatformPlayStatus status;
            check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
            alloc_response(IAPExtendedRetPlayStatusPayload);
            response->state          = status.state; /* TODO: convert enum */
            response->track_pos_ms   = swap_32(status.track_pos_ms);
            response->track_total_ms = swap_32(status.track_total_ms);
            return IAPExtendedInterfaceCommandID_ReturnPlayStatus;
        } break;
        case IAPExtendedInterfaceCommandID_GetCurrentPlayingTrackIndex: {
            struct IAPPlatformPlayStatus status;
            check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
            alloc_response(IAPReturnCurrentPlayingTrackIndexPayload);
            response->index = swap_32(status.state == IAPIPodStatePlayStatus_PlaybackStopped ? (uint32_t)-1 : status.track_index);
            return IAPExtendedInterfaceCommandID_ReturnCurrentPlayingTrackIndex;
        } break;
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackTitle: {
            read_request(IAPGetIndexedPlayingTrackStringPayload);
            struct IAPPlatformTrackInfo info = {.title = response_span};
            check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->index), &info), -IAPAckStatus_ECommandFailed);
            return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackTitle;
        } break;
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackArtistName: {
            read_request(IAPGetIndexedPlayingTrackStringPayload);
            struct IAPPlatformTrackInfo info = {.artist = response_span};
            check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->index), &info), -IAPAckStatus_ECommandFailed);
            return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackArtistName;
        } break;
        case IAPExtendedInterfaceCommandID_GetIndexedPlayingTrackAlbumName: {
            read_request(IAPGetIndexedPlayingTrackStringPayload);
            struct IAPPlatformTrackInfo info = {.album = response_span};
            check_ret(iap_platform_get_indexed_track_info(ctx, swap_32(request->index), &info), -IAPAckStatus_ECommandFailed);
            return IAPExtendedInterfaceCommandID_ReturnIndexedPlayingTrackAlbumName;
        } break;
        case IAPExtendedInterfaceCommandID_SetPlayStatusChangeNotification: {
            if(request_span->size == sizeof(struct IAPSetPlayStatusChangeNotification1BytePayload)) {
                read_request(IAPSetPlayStatusChangeNotification1BytePayload);
                ctx->notifications_4 = 0;
                if(request->enable) {
                    ctx->enabled_notifications_4 = 1 << IAPStatusChangeNotificationType_PlaybackStopped |
                                                   1 << IAPStatusChangeNotificationType_TrackIndex |
                                                   1 << IAPStatusChangeNotificationType_PlaybackFEWSeekStop |
                                                   1 << IAPStatusChangeNotificationType_PlaybackREWSeekStop |
                                                   1 << IAPStatusChangeNotificationType_TrackTimeOffsetMSec |
                                                   1 << IAPStatusChangeNotificationType_ChapterIndex;
                } else {
                    ctx->enabled_notifications_4 = 0;
                }
            } else if(request_span->size == sizeof(struct IAPSetPlayStatusChangeNotification4BytesPayload)) {
                read_request(IAPSetPlayStatusChangeNotification4BytesPayload);
                ctx->enabled_notifications_4 = play_stage_change_notification_set_mask_to_type_mask(swap_32(request->mask));
            }
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_PlayCurrentSelection: {
            read_request(IAPPlayCurrentSelectionPayload);
            const struct IAPPlatformPendingControl pending = {
                .req_command = command,
                .ack_command = IAPExtendedInterfaceCommandID_IPodAck,
                .trans_id    = ctx->handling_trans_id,
                .lingo       = lingo,
            };
            iap_platform_control(ctx, IAPPlatformControl_Play, pending);
            response_span->ptr = NULL;
            return 0;
        } break;
        case IAPExtendedInterfaceCommandID_PlayControl: {
            read_request(IAPPlayControlPayload);
            static const int enum_table[][2] = {
                {IAPPlayControlCode_TogglePlayPause, IAPPlatformControl_TogglePlayPause},
                {IAPPlayControlCode_Stop, IAPPlatformControl_Stop},
                {IAPPlayControlCode_NextTrack, IAPPlatformControl_Next},
                {IAPPlayControlCode_PrevTrack, IAPPlatformControl_Prev},
                {IAPPlayControlCode_StartFF, -1},
                {IAPPlayControlCode_StartRew, -1},
                {IAPPlayControlCode_EndFFRew, -1},
                {IAPPlayControlCode_Next, IAPPlatformControl_Next},
                {IAPPlayControlCode_Prev, IAPPlatformControl_Prev},
                {IAPPlayControlCode_Play, IAPPlatformControl_Play},
                {IAPPlayControlCode_Pause, IAPPlatformControl_Pause},
                {IAPPlayControlCode_NextChapter, IAPPlatformControl_Next},
                {IAPPlayControlCode_PrevChapter, IAPPlatformControl_Prev},
                {IAPPlayControlCode_ResumeIPod, -1},
            };
            int control = -1;
            for(size_t i = 0; i < array_size(enum_table); i += 1) {
                if(enum_table[i][0] == request->code) {
                    control = enum_table[i][1];
                    break;
                }
            }
            if(control >= 0) {
                const struct IAPPlatformPendingControl pending = {
                    .req_command = command,
                    .ack_command = IAPExtendedInterfaceCommandID_IPodAck,
                    .trans_id    = ctx->handling_trans_id,
                    .lingo       = lingo,
                };
                iap_platform_control(ctx, control, pending);
                response_span->ptr = NULL;
                return 0;
            }
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_GetShuffle: {
            alloc_response(IAPReturnShufflePayload);
            check_ret(iap_platform_get_shuffle_setting(ctx, &response->mode), -IAPAckStatus_ECommandFailed);
            return IAPExtendedInterfaceCommandID_ReturnShuffle;
        } break;
        case IAPExtendedInterfaceCommandID_SetShuffle: {
            read_request(IAPSetShufflePayload);
            check_ret(iap_platform_set_shuffle_setting(ctx, request->mode), -IAPAckStatus_ECommandFailed);
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_GetRepeat: {
            alloc_response(IAPReturnRepeatPayload);
            check_ret(iap_platform_get_repeat_setting(ctx, &response->mode), -IAPAckStatus_ECommandFailed);
            return IAPExtendedInterfaceCommandID_ReturnRepeat;
        } break;
        case IAPExtendedInterfaceCommandID_SetRepeat: {
            read_request(IAPSetRepeatPayload);
            check_ret(iap_platform_set_repeat_setting(ctx, request->mode), -IAPAckStatus_ECommandFailed);
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_SetDisplayImage: {
            /* TODO: pass downloaded image to user */
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_GetNumPlayingTracks: {
            struct IAPPlatformPlayStatus status;
            check_ret(iap_platform_get_play_status(ctx, &status), -IAPAckStatus_ECommandFailed);
            alloc_response(IAPRetNumPlayingTracksPayload);
            response->num_playing_tracks = swap_32(status.track_count);
            return IAPExtendedInterfaceCommandID_ReturnNumPlayingTracks;
        } break;
        case IAPExtendedInterfaceCommandID_SetCurrentPlayingTrack: {
            read_request(IAPSetCurrentPlayingTrackPayload);
            check_ret(iap_platform_set_playing_track(ctx, swap_32(request->index)), -IAPAckStatus_ECommandFailed);
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_SelectSortDBRecord: {
            /* ignored */
            return ipod_ack_ext(command, IAPAckStatus_Success, response_span);
        } break;
        case IAPExtendedInterfaceCommandID_GetColorDisplayImageLimits: {
            alloc_response(IAPColorDisplayImageLimit);
            response->max_width    = 0;
            response->max_height   = 0;
            response->pixel_format = IAPArtworkPixelFormats_RGB565LE;
            return IAPExtendedInterfaceCommandID_ReturnColorDisplayImageLimits;
        } break;
        }
        break;
    case IAPLingoID_DigitalAudio:
        switch(command) {
        case IAPDigitalAudioCommandID_AccessoryAck: {
            read_request(IAPAccAckPayload);
            response_span->ptr = NULL;
            if(request->id == IAPDigitalAudioCommandID_TrackNewAudioAttributes) {
                check_ret(ctx->waiting_for_audio_attrs_ack, 0, "unexpected ack");
                ctx->waiting_for_audio_attrs_ack = iap_false;
            }
            check_ret(request->status == IAPAckStatus_Success, 0);
            return 0;
        } break;
        case IAPDigitalAudioCommandID_RetAccessorySampleRateCaps: {
            check_ret(iap_platform_on_acc_samprs_received(ctx, request_span), -IAPAckStatus_ECommandFailed);
            response_span->ptr = NULL; /* no response */
            return 0;
        } break;
        }
        break;
    }

    return -IAPAckStatus_EUnknownID;
}

static IAPBool transition_idps_to_auth_cb(struct IAPContext* ctx) {
    print("starting accessory authentication");
    ctx->phase = IAPPhase_Auth;
    check_ret(_iap_send_packet(ctx, IAPLingoID_General, IAPGeneralCommandID_GetAccessoryAuthenticationInfo, _iap_next_trans_id(ctx), _iap_get_buffer_for_send_payload(ctx).ptr), iap_false);
    return iap_true;
}

static int32_t handle_in_connected(struct IAPContext* ctx, uint8_t lingo, uint16_t command, struct IAPSpan* request_span, struct IAPSpan* response_span) {
    switch(lingo) {
    case IAPLingoID_General:
        switch(command) {
        case IAPGeneralCommandID_IdentifyDeviceLingoes: {
            read_request(IAPIdentifyDeviceLingoesPayload);
            switch(swap_32(request->options)) {
            case IAPIdentifyDeviceLingoesOptions_NoAuth:
                break;
            case IAPIdentifyDeviceLingoesOptions_DeferAuth:
                warn("unsupported option 0x%04" PRIX32, swap_32(request->options));
                return -IAPAckStatus_EBadParameter;
            case IAPIdentifyDeviceLingoesOptions_ImmediateAuth:
                ctx->on_send_complete = transition_idps_to_auth_cb;
                break;
            }
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
        } break;
        case IAPGeneralCommandID_StartIDPS: {
            ctx->phase = IAPPhase_IDPS;
            return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
        } break;
        }
        break;
    }
    return -IAPAckStatus_EUnknownID;
}

static int32_t handle_in_idps(struct IAPContext* ctx, uint8_t lingo, uint16_t command, struct IAPSpan* request_span, struct IAPSpan* response_span) {
    switch(lingo) {
    case IAPLingoID_General:
        switch(command) {
        case IAPGeneralCommandID_SetFIDTokenValues: {
            const int ret = _iap_hanlde_set_fid_token_values(request_span, response_span);
            check_ret(ret == 0, ret);
            return IAPGeneralCommandID_AckFIDTokenValues;
        } break;
        case IAPGeneralCommandID_EndIDPS: {
            read_request(IAPEndIDPSPayload);
            check_ret(request->status == IAPEndIDPSStatus_Success, -IAPAckStatus_ECommandFailed);
            ctx->on_send_complete = transition_idps_to_auth_cb;
            alloc_response(IAPIDPSStatusPayload);
            response->status = IAPIDPSStatus_Success;
            return IAPGeneralCommandID_IDPSStatus;
        } break;
        }
        break;
    }
    return iap_false;
}

static IAPBool send_auth_challenge_sig_cb(struct IAPContext* ctx) {
    check_ret(ctx->phase == IAPPhase_Auth, iap_false);
    struct IAPSpan                     request_span = _iap_get_buffer_for_send_payload(ctx);
    struct IAPGetAccAuthSigPayload2p0* request      = iap_span_alloc(&request_span, sizeof(*request));
    check_ret(request != NULL, iap_false);
    request->retry = 1;
    check_ret(_iap_send_packet(ctx, IAPLingoID_General, IAPGeneralCommandID_GetAccessoryAuthenticationSignature, _iap_next_trans_id(ctx), request_span.ptr), iap_false);
    return iap_true;
}

static IAPBool send_sample_rate_caps_cb(struct IAPContext* ctx) {
    struct IAPSpan request = _iap_get_buffer_for_send_payload(ctx);
    check_ret(_iap_send_packet(ctx, IAPLingoID_DigitalAudio, IAPDigitalAudioCommandID_GetAccessorySampleRateCaps, _iap_next_trans_id(ctx), request.ptr), iap_false);
    return iap_true;
}

static int32_t handle_in_auth(struct IAPContext* ctx, uint8_t lingo, uint16_t command, struct IAPSpan* request_span, struct IAPSpan* response_span) {
    switch(lingo) {
    case IAPLingoID_General:
        switch(command) {
        case IAPGeneralCommandID_RetAccessoryAuthenticationInfo: {
            read_request(IAPRetAccAuthInfoPayload2p0);
            print("accessory cert %" PRIu8 "/%" PRIu8, request->cert_current_section_index, request->cert_max_section_index);
            /* iap_platform_dump_hex(request->ptr, request->size); */
            if(request->cert_current_section_index < request->cert_max_section_index) {
                return ipod_ack(command, IAPAckStatus_Success, response_span, IAPGeneralCommandID_IPodAck);
            } else {
                ctx->on_send_complete = send_auth_challenge_sig_cb;
                alloc_response(IAPAckAccAuthInfoPayload);
                response->status = IAPAckAccAuthInfoStatus_Supported;
                return IAPGeneralCommandID_AckAccessoryAuthenticationInfo;
            }
        } break;
        case IAPGeneralCommandID_RetAccessoryAuthenticationSignature: {
            print("accessory signature");
            /* iap_platform_dump_hex(request->ptr, request->size); */

            alloc_response(IAPAckAccAuthSigPayload);
            response->status = IAPAckStatus_Success;

            ctx->phase            = IAPPhase_Authed;
            ctx->on_send_complete = send_sample_rate_caps_cb;

            return IAPGeneralCommandID_AckAccessoryAuthenticationStatus;
        } break;
        }
        break;
    }
    return -IAPAckStatus_EUnknownID;
}

static int32_t build_ipod_ack_response(uint8_t lingo, uint16_t command, uint8_t status, struct IAPSpan* response_span) {
    static int32_t ack_commmand_ids[] = {
        IAPGeneralCommandID_IPodAck,
        -1,
        IAPSimpleRemoteCommandID_IPodAck,
        IAPDisplayRemoteCommandID_IPodAck,
        IAPExtendedInterfaceCommandID_IPodAck,
        -1,
        IAPUSBHostModeCommandID_IPodAck,
        -1,
        -1,
        IAPSportsCommandID_IPodAck,
        IAPDigitalAudioCommandID_IPodAck,
        -1,
        IAPStorageCommandID_IPodAck,
        IAPIPodOutCommandID_IPodAck,
        IAPLocationCommandID_IPodAck,
    };
    check_ret(lingo < array_size(ack_commmand_ids) && ack_commmand_ids[lingo] >= 0, -1);
    switch(lingo) {
    case IAPLingoID_General:
    case IAPLingoID_SimpleRemote:
    case IAPLingoID_DisplayRemote:
    case IAPLingoID_USBHostMode:
    case IAPLingoID_Sports:
    case IAPLingoID_DigitalAudio:
    case IAPLingoID_IPodOut:
    case IAPLingoID_Location: {
        alloc_response(IAPIPodAckPayload);
        response->id     = command;
        response->status = status;
    } break;
    case IAPLingoID_ExtendedInterface: {
        alloc_response(IAPExtendedIPodAckPayload);
        response->id     = swap_16(command);
        response->status = status;
    } break;
    case IAPLingoID_Storage: {
        alloc_response(IAPStorageIPodAckPayload);
        response->id     = command;
        response->status = status;
        response->handle = -1; /* TODO: set proper handle */
    } break;
    }
    return ack_commmand_ids[lingo];
}

IAPBool _iap_feed_packet(struct IAPContext* ctx, const uint8_t* const data, const size_t size) {
    union {
        uint8_t  u8;
        uint16_t u16;
    } buf;
    struct IAPSpan span = {(uint8_t*)data, size};

    /* read sof byte */
    check_ret(iap_span_read_8(&span, &buf.u8), iap_false);
    if(buf.u8 == IAP_SYNC_BYTE) {
        /* skip sync byte */
        check_ret(iap_span_read_8(&span, &buf.u8), iap_false);
    }
    check_ret(buf.u8 == IAP_SOF_BYTE, iap_false, "%x != %x", buf.u8, IAP_SOF_BYTE);
    const uint8_t* const checksum_range_begin = span.ptr;
    /* read size */
    check_ret(iap_span_read_8(&span, &buf.u8), iap_false);
    size_t length;
    if(buf.u8 == 0) {
        /* long packet */
        check_ret(iap_span_read_16(&span, &buf.u16), iap_false);
        length = buf.u16;
    } else {
        length = buf.u8;
    }
    /* we have length, strip span so that it contains lingo,command,payload */
    check_ret(span.size >= length + 1 /* checksum */, iap_false);
    span.size = length;
    /* verify checksum */
    const uint8_t* const checksum_range_end = span.ptr + span.size + 1 /* checksum */;
    uint8_t              checksum           = 0;
    for(const uint8_t* ptr = checksum_range_begin; ptr < checksum_range_end; ptr += 1) {
        checksum += *ptr;
    }
    check_ret(checksum == 0, iap_false);
    /* read lingo id */
    check_ret(iap_span_read_8(&span, &buf.u8), iap_false);
    uint8_t lingo = buf.u8;
    /* read command id */
    uint16_t command;
    if(lingo == IAPLingoID_ExtendedInterface) {
        check_ret(iap_span_read_16(&span, &buf.u16), iap_false);
        command = buf.u16;
    } else {
        check_ret(iap_span_read_8(&span, &buf.u8), iap_false);
        command = buf.u8;
    }

    /* now span contains only payload */

    /* request handling */
    if(ctx->trans_id_support == TransIDUnknown) {
        check_ret(lingo == IAPLingoID_General, iap_false);
        if(command == IAPGeneralCommandID_StartIDPS) {
            ctx->trans_id_support = TransIDSupported;
        } else if(command == IAPGeneralCommandID_IdentifyDeviceLingoes) {
            ctx->trans_id_support  = TransIDNotSupported;
            ctx->handling_trans_id = -1;
        } else {
            warn("the first command(%02X:%04X) must be StartIDPS or IdentifyDeviceLingoes", lingo, command);
            return iap_false;
        }
    }
    if(ctx->trans_id_support == TransIDSupported) {
        check_ret(iap_span_read_16(&span, &buf.u16), iap_false);
        ctx->handling_trans_id = buf.u16;
    }

    if(ctx->opts.enable_packet_dump) {
        IAP_LOGF("==== acc ====");
        _iap_dump_packet(lingo, command, ctx->handling_trans_id, span);
    }

    struct IAPSpan response = _iap_get_buffer_for_send_payload(ctx);
    int32_t        ret      = handle_command(ctx, lingo, command, &span, &response);
    if(response.ptr == NULL) {
        /* handler disabled response */
        return iap_true;
    }
    if(ret >= 0) {
        /* handled successfully */
        goto respond;
    }
    if(ret != -IAPAckStatus_EUnknownID) {
        /* handled, but error */
        goto error_ack;
    }

    /* not a standard request, try authentication handlers */
    ret      = -IAPAckStatus_EBadParameter;
    response = _iap_get_buffer_for_send_payload(ctx);
    switch(ctx->phase) {
    case IAPPhase_Connected:
        ret = handle_in_connected(ctx, lingo, command, &span, &response);
        break;
    case IAPPhase_IDPS:
        ret = handle_in_idps(ctx, lingo, command, &span, &response);
        break;
    case IAPPhase_Auth:
        ret = handle_in_auth(ctx, lingo, command, &span, &response);
        break;
    }
    if(response.ptr == NULL) {
        /* handler disabled response */
        return iap_true;
    }
    if(ret >= 0) {
        /* handled successfully */
        goto respond;
    }
error_ack:
    /* handling failed, replace response with ipod ack */
    warn("command handling failed 0x%02X(%s):0x%04X", lingo, _iap_lingo_str(lingo), command);
    response = _iap_get_buffer_for_send_payload(ctx);
    ret      = build_ipod_ack_response(lingo, command, -ret, &response);
    check_ret(ret >= 0, iap_false);
respond:
    check_ret(_iap_send_packet(ctx, lingo, ret, ctx->handling_trans_id, response.ptr), iap_false);
    return iap_true;
}

struct IAPSpan _iap_get_buffer_for_send_payload(struct IAPContext* ctx) {
    const size_t header_size = 1 /* sof */ +
                               3 /* long format length */ +
                               1 /* lingo */ +
                               2 /* largest command id */ +
                               2 /* trans id */;
    const size_t footer_size = 1 /* checksum */;

    struct IAPSpan buf = {ctx->send_buf + header_size, SEND_BUFFER_SIZE - header_size - footer_size};
    return buf;
}

int32_t _iap_next_trans_id(struct IAPContext* ctx) {
    if(ctx->trans_id_support == TransIDSupported) {
        return ctx->trans_id += 1;
    } else {
        return -1;
    }
}

IAPBool _iap_send_packet(struct IAPContext* ctx, uint8_t lingo, uint16_t command, int32_t trans_id, uint8_t* final_ptr) {
    uint8_t* ptr          = _iap_get_buffer_for_send_payload(ctx).ptr;
    size_t   payload_size = final_ptr - ptr;

    if(ctx->opts.enable_packet_dump) {
        IAP_LOGF("==== dev ====");
        struct IAPSpan payload_span = {
            .ptr  = _iap_get_buffer_for_send_payload(ctx).ptr,
            .size = payload_size,
        };
        _iap_dump_packet(lingo, command, trans_id, payload_span);
    }

#define pack_8(val) \
    ptr -= 1;       \
    *(uint8_t*)ptr = val;
#define pack_16(val) \
    ptr -= 2;        \
    *(uu16*)ptr = swap_16(val);

    /* fill header in reverse order */
    /* trans id */
    if(trans_id >= 0) {
        pack_16(trans_id);
    }
    /* command id */
    if(lingo == IAPLingoID_ExtendedInterface) {
        pack_16(command);
    } else {
        pack_8(command);
    }
    /* lingo */
    pack_8(lingo);
    /* length */
    const uint16_t length = 1 /*lingo*/ + (lingo == IAPLingoID_ExtendedInterface ? 2 : 1) /*command*/ + (trans_id >= 0 ? 2 : 0) + payload_size;
    if(length <= 0xFC) {
        pack_8(length);
    } else {
        pack_16(length);
        pack_8(0);
    }
    /* sof */
    pack_8(IAP_SOF_BYTE);

    /* set checksum */
    uint8_t checksum = 0;
    for(uint8_t* p = ptr + 1 /* exclude sof byte */; p < final_ptr; p += 1) {
        checksum += *p;
    }
    checksum *= -1;
    *final_ptr = checksum;

    check_ret(_iap_send_hid_reports(ctx, ptr - ctx->send_buf, final_ptr - ctx->send_buf + 1 /* include checksum */), iap_false);
    return iap_true;
}

static IAPBool push_active_event(struct IAPContext* ctx, struct IAPActiveEvent event) {
    if(!ctx->send_busy) {
        check_ret(event.callback(ctx, &event), iap_false);
        return iap_true;
    }

    for(size_t i = 0; i < array_size(ctx->active_events); i += 1) {
        if(ctx->active_events[i].callback == NULL) {
            ctx->active_events[i] = event;
            return iap_true;
        }
    }
    return iap_false;
}

static IAPBool process_select_sampr(struct IAPContext* ctx, struct IAPActiveEvent* event) {
    if(ctx->waiting_for_audio_attrs_ack) {
        IAP_ERRORF("another sampr request pending");
    }
    ctx->waiting_for_audio_attrs_ack = iap_true;

    struct IAPSpan                            request_span = _iap_get_buffer_for_send_payload(ctx);
    struct IAPTrackNewAudioAttributesPayload* request      = iap_span_alloc(&request_span, sizeof(*request));
    request->sample_rate                                   = swap_32(event->sampr);
    request->sound_check                                   = 0;
    request->volume_adjustment                             = 0;
    check_ret(_iap_send_packet(ctx, IAPLingoID_DigitalAudio, IAPDigitalAudioCommandID_TrackNewAudioAttributes, _iap_next_trans_id(ctx), request_span.ptr), iap_false);
    return iap_true;
}

IAPBool iap_select_sampr(struct IAPContext* ctx, uint32_t sampr) {
    struct IAPActiveEvent event = {
        .callback = process_select_sampr,
        .sampr    = sampr,
    };
    check_ret(push_active_event(ctx, event), iap_false);
    return iap_true;
}

static IAPBool process_control_response(struct IAPContext* ctx, struct IAPActiveEvent* event) {
    struct IAPSpan                          request_span = _iap_get_buffer_for_send_payload(ctx);
    const struct IAPPlatformPendingControl* ctrl         = &event->control_response.control;
    const uint8_t                           status       = event->control_response.result ? IAPAckStatus_Success : IAPAckStatus_ECommandFailed;
    if(ctrl->lingo == IAPLingoID_ExtendedInterface) {
        ipod_ack_ext(ctrl->req_command, status, &request_span);
    } else {
        ipod_ack(ctrl->req_command, status, &request_span, ctrl->ack_command);
    }
    check_ret(_iap_send_packet(ctx, ctrl->lingo, ctrl->ack_command, ctrl->trans_id, request_span.ptr), iap_false);
    return iap_true;
}

IAPBool iap_control_response(struct IAPContext* ctx, struct IAPPlatformPendingControl pending, IAPBool result) {
    struct IAPActiveEvent event = {
        .callback         = process_control_response,
        .control_response = {pending, result},
    };
    if(pending.lingo == IAPLingoID_SimpleRemote && pending.req_command == IAPSimpleRemoteCommandID_ContextButtonStatus) {
        /* no response */
        return iap_true;
    }
    check_ret(push_active_event(ctx, event), iap_false);

    return iap_true;
}
