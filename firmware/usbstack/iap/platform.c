/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "audio.h"
#include "buffering.h"
#include "core_alloc.h"
#include "metadata.h"
#include "misc.h"
#include "pcm_mixer.h"
#include "pcm_sink.h"
#include "playback.h"
#include "playlist.h"
#include "powermgmt.h"
#include "settings.h"
#include "sound.h"
#include "usb_drv.h"

#include "../usb_iap.h"
#include "debug.h"
#include "libiap/iap.h"
#include "macros.h"
#include "platform.h"

void* iap_platform_malloc(struct IAPContext* iap_ctx, size_t size, int flags) {
    struct Platform* plt = iap_ctx->platform;
    for(size_t i = 0; i < ARRAYLEN(plt->malloc_results); i += 1) {
        if(plt->malloc_results[i].ptr != NULL) {
            continue;
        }
        struct IAPAllocResult result;
        if(flags & IAPPlatformMallocFlags_Uncached) {
            check_act(iap_alloc_usb_send_buffer(size, &result), return NULL);
        } else {
            check_act(iap_alloc_buffer(size, &result), return NULL);
        }
        plt->malloc_results[i] = result;
        return result.ptr;
    }
    ERROR("no free malloc slot");
    return NULL;
}

void iap_platform_free(struct IAPContext* iap_ctx, void* ptr) {
    struct Platform* plt = iap_ctx->platform;
    for(size_t i = 0; i < ARRAYLEN(plt->malloc_results); i += 1) {
        if(plt->malloc_results[i].ptr == ptr) {
            core_free(plt->malloc_results[i].handle);
            plt->malloc_results[i].ptr = NULL;
            return;
        }
    }
    ERROR("no matching malloc slot");
}

int iap_platform_send_hid_report(struct IAPContext* iap_ctx, const void* ptr, size_t size) {
    (void)iap_ctx;
#if DEBUG_DUMP_TX == 1
    logf("==== dev ==== %p %u > %d", ptr, size, HID_EP_IN);
    iap_platform_dump_hex(ptr, MIN(size, 48));
#endif
    const int ret = usb_drv_send_nonblocking(HID_EP_IN, (void*)ptr, size);
    return ret == 0 ? (int)size : ret;
}

IAPBool iap_platform_get_ipod_serial_num(struct IAPContext* iap_ctx, struct IAPSpan* serial) {
    (void)iap_ctx;
    static const char* serial_num = "000000000000";
    return iap_span_append(serial, serial_num, strlen(serial_num) + 1);
}

IAPBool iap_platform_get_play_status(struct IAPContext* iap_ctx, struct IAPPlatformPlayStatus* status) {
    struct Platform* plt = iap_ctx->platform;

    status->state = _iap_convert_play_status(audio_status());
    if(status->state == IAPIPodStatePlayStatus_PlaybackStopped) {
        return iap_true;
    }

    struct mp3entry* id3 = audio_current_track();
    check_act(id3 != NULL, return iap_false);
    status->track_total_ms = id3->length;
    status->track_pos_ms   = id3->elapsed;
    status->track_index    = playlist_get_display_index() - 1;
    status->track_count    = playlist_amount();
    status->track_caps     = IAPIPodStateTrackCapBits_HasReleaseDate;
    if(plt->aa_slot >= 0 && playback_current_aa_hid(plt->aa_slot) >= 0) {
        status->track_caps |= IAPIPodStateTrackCapBits_HasAlbumArts;
    }

    return iap_true;
}

void iap_platform_control(struct IAPContext* iap_ctx, enum IAPPlatformControl control, struct IAPPlatformPendingControl pending) {
    struct Platform* plt = iap_ctx->platform;

    long button = BUTTON_NONE;
    switch(control) {
    case IAPPlatformControl_TogglePlayPause:
        button = BUTTON_MULTIMEDIA_PLAYPAUSE;
        break;
    case IAPPlatformControl_Play:
        if(audio_status() != AUDIO_STATUS_PLAY) {
            button = BUTTON_MULTIMEDIA_PLAYPAUSE;
        }
        break;
    case IAPPlatformControl_Pause:
        if(audio_status() == AUDIO_STATUS_PLAY) {
            button = BUTTON_MULTIMEDIA_PLAYPAUSE;
        }
        break;
    case IAPPlatformControl_Stop:
        button = BUTTON_MULTIMEDIA_STOP;
        break;
    case IAPPlatformControl_Next:
        button = BUTTON_MULTIMEDIA_NEXT;
        break;
    case IAPPlatformControl_Prev:
        button = BUTTON_MULTIMEDIA_PREV;
        break;
    case IAPPlatformControl_VolumeUp:
        adjust_volume(1);
        break;
    case IAPPlatformControl_VolumeDown:
        adjust_volume(-1);
        break;
    case IAPPlatformControl_ToggleMute:
        /* rockbox has no mute/unmute */
        break;
    }
    logf("control=%d button=0x%04lX", control, button);

    IAPBool ret = iap_true;
    if(button == BUTTON_NONE) {
        goto exit;
    }
    check_act(button_queue_try_post(button, 0), ret = false);
    if(ret && button == BUTTON_MULTIMEDIA_PLAYPAUSE && audio_status() == 0) {
        /* we are transitioning to stopped to playing, which may take time.
         * to maintain synchronization with accessories, do not send an ack until playback actually begins. */
        plt->control_pending = true;
        plt->pending_control = pending;
        return;
    }
exit:
    check_act(iap_control_response(iap_ctx, pending, ret), );
}

static uint8_t normalize_8(int val, int min, int max) {
    return 0xFF * (val - min) / (max - min);
}

IAPBool iap_platform_get_volume(struct IAPContext* iap_ctx, struct IAPPlatformVolumeStatus* status) {
    (void)iap_ctx;

    status->volume = _iap_convert_volume(global_status.volume);
    status->muted  = iap_false;
    return iap_true;
}

IAPBool iap_platform_get_power_status(struct IAPContext* iap_ctx, struct IAPPlatformPowerStatus* status) {
    (void)iap_ctx;
    status->battery_level = _iap_convert_battery_level(battery_level());
    status->state         = _iap_convert_charge_status(charge_state);
    return iap_true;
}

IAPBool iap_platform_get_shuffle_setting(struct IAPContext* iap_ctx, uint8_t* status) {
    (void)iap_ctx;
    *status = _iap_convert_shuffle_state(global_settings.playlist_shuffle);
    return iap_true;
}

IAPBool iap_platform_set_shuffle_setting(struct IAPContext* iap_ctx, uint8_t status) {
    (void)iap_ctx;

    if(status == IAPIPodStateShuffleSettingState_Tracks && !global_settings.playlist_shuffle) {
        global_settings.playlist_shuffle = true;
        settings_save();
        if(audio_status() & AUDIO_STATUS_PLAY) {
            playlist_randomise(NULL, current_tick, true);
        }
    } else if(status == IAPIPodStateShuffleSettingState_Off && global_settings.playlist_shuffle) {
        global_settings.playlist_shuffle = false;
        settings_save();
        if(audio_status() & AUDIO_STATUS_PLAY) {
            playlist_sort(NULL, true);
        }
    }
    return iap_true;
}

IAPBool iap_platform_get_repeat_setting(struct IAPContext* iap_ctx, uint8_t* status) {
    (void)iap_ctx;
    *status = _iap_convert_repeat_state(global_settings.repeat_mode);
    return iap_true;
}

IAPBool iap_platform_set_repeat_setting(struct IAPContext* iap_ctx, uint8_t status) {
    (void)iap_ctx;

    /* there are more repeat options in Rockbox rather than iAP.
     * e.g. RB Repeat One and AB are both mapped to iAP Repeat One.
     * so we should not accept iAP Repeat One as RB Repeat One, if we have
     * RB Repeat AB set. */
    if(_iap_convert_repeat_state(global_settings.repeat_mode) == status) {
        /* already the same (or equievalent) mode set */
        return iap_true;
    }

    static const uint8_t table[] = {
        [IAPIPodStateRepeatSettingState_Off] = REPEAT_OFF,
        [IAPIPodStateRepeatSettingState_One] = REPEAT_ONE,
        [IAPIPodStateRepeatSettingState_All] = REPEAT_ALL,
    };
    check_act(status < ARRAY_SIZE(table), return iap_false);
    global_settings.repeat_mode = table[status];
    settings_save();
    if(audio_status() & AUDIO_STATUS_PLAY) {
        audio_flush_and_reload_tracks();
    }
    return iap_true;
}

IAPBool iap_platform_get_date_time(struct IAPContext* iap_ctx, struct IAPDateTime* time) {
    (void)iap_ctx;
    _iap_convert_datetime(get_time(), time);
    return iap_true;
}

IAPBool iap_platform_get_backlight_level(struct IAPContext* iap_ctx, uint8_t* level) {
    (void)iap_ctx;

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    *level = normalize_8(global_settings.brightness, MIN_BRIGHTNESS_SETTING, MAX_BRIGHTNESS_SETTING);
#else
    *level = 0xFF;
#endif
    return iap_true;
}

IAPBool iap_platform_get_hold_switch_state(struct IAPContext* iap_ctx, IAPBool* state) {
    (void)iap_ctx;

#ifdef HAS_BUTTON_HOLD
    *state = button_hold();
#else
    *state = iap_false;
#endif
    return iap_true;
}

/* taken from iap-core.c */
static void get_trackinfo(const unsigned int track, struct mp3entry* id3) {
    int tracknum = track;
    tracknum += playlist_get_first_index(NULL);
    if(tracknum >= playlist_amount()) {
        tracknum -= playlist_amount();
    }

    if(playlist_next(0) != tracknum) {
        struct playlist_track_info info;
        playlist_get_track_info(NULL, tracknum, &info);
        get_metadata(id3, -1, info.filename);
    } else {
        memcpy(id3, audio_current_track(), sizeof(*id3));
    }
}

IAPBool iap_platform_get_indexed_track_info(struct IAPContext* iap_ctx, uint32_t index, struct IAPPlatformTrackInfo* info) {
    struct Platform* plt = iap_ctx->platform;

    struct playlist_track_info track;
    struct mp3entry            id3;
    check_act(playlist_get_track_info(NULL, index, &track) == 0, return iap_false);
    get_trackinfo(index, &id3);

    if(info->total_ms != NULL) {
        *info->total_ms = id3.length;
    }
    if(info->caps != NULL) {
        *info->caps = IAPIPodStateTrackCapBits_HasReleaseDate;
        /* FIXME: respect index */
        if(plt->aa_slot >= 0 && playback_current_aa_hid(plt->aa_slot) >= 0) {
            *info->caps |= IAPIPodStateTrackCapBits_HasAlbumArts;
        }
    }
    if(info->release_date != NULL) {
        info->release_date->year    = id3.year;
        info->release_date->month   = 0;
        info->release_date->day     = 0;
        info->release_date->hour    = 0;
        info->release_date->minute  = 0;
        info->release_date->seconds = 0;
    }
    if(info->artist != NULL) {
        check_act(iap_span_append(info->artist, id3.artist, strlen(id3.artist) + 1), return iap_false);
    }
    if(info->composer != NULL) {
        check_act(iap_span_append(info->composer, id3.composer, strlen(id3.composer) + 1), return iap_false);
    }
    if(info->album != NULL) {
        check_act(iap_span_append(info->album, id3.album, strlen(id3.album) + 1), return iap_false);
    }
    if(info->title != NULL) {
        check_act(iap_span_append(info->title, id3.title, strlen(id3.title) + 1), return iap_false);
    }
    return iap_true;
}

IAPBool iap_platform_set_playing_track(struct IAPContext* iap_ctx, uint32_t index) {
    (void)iap_ctx;
    audio_skip((int)index - playlist_next(0));
    return iap_true;
}

IAPBool iap_platform_open_artwork(struct IAPContext* iap_ctx, uint32_t index, struct IAPPlatformArtwork* artwork) {
    struct Platform* plt = iap_ctx->platform;
    /* only aa for currently playing track is available */
    check_act((int)index == playlist_get_display_index() - 1, return iap_false);
    const int hid = playback_current_aa_hid(plt->aa_slot);
    check_act(hid >= 0, return iap_false, "%d %d", plt->aa_slot, hid);
    struct bitmap* bmp;
    check_act(bufgetdata(hid, 0, (void*)&bmp) > 0, return iap_false);
    artwork->color  = iap_true;
    artwork->width  = bmp->width;
    artwork->height = bmp->height;
    artwork->opaque = hid;
    return iap_true;
}

IAPBool iap_platform_get_artwork_ptr(struct IAPContext* iap_ctx, struct IAPPlatformArtwork* artwork, struct IAPSpan* span) {
    struct Platform* plt = iap_ctx->platform;

    /* check the albumart has not reloaded */
    /* FIXME: not a correct check due to possibility of hid confliction */
    const int hid = playback_current_aa_hid(plt->aa_slot);
    check_act(hid == (int)artwork->opaque, return iap_false);

    struct bitmap* bmp;
    /* more checks */
    check_act(bufgetdata(hid, 0, (void*)&bmp) > 0, return iap_false);
    check_act(bmp->width == artwork->width && bmp->height == artwork->height, return iap_false);

    span->ptr  = bmp->data;
    span->size = bmp->width * bmp->height * 2;
    return iap_true;
}

IAPBool iap_platform_close_artwork(struct IAPContext* iap_ctx, struct IAPPlatformArtwork* artwork) {
    (void)iap_ctx;
    (void)artwork;
    return iap_true;
}

void iap_platform_dump_hex(const void* ptr, size_t size) {
    if(ptr == NULL) {
        logf("(null)");
        return;
    }

#if DEBUG_HEXDUMP_NOLIMIT != 1
    size = MIN(size, 32);
#endif

    static const char chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    char line[4 * (8 + 1) + 1];
    for(size_t l = 0; l * 16 < size; l += 1) {
        char* c = line;
        for(size_t b = 0; b < 4; b += 1) {
            for(size_t i = 0; i < 4; i += 1) {
                size_t index = l * 16 + b * 4 + i;
                if(index >= size) {
                    break;
                }
                *c++ = chars[((uint8_t*)ptr)[index] >> 4];
                *c++ = chars[((uint8_t*)ptr)[index] & 0xF];
            }
            *c++ = ' ';
        }
        *c++ = '\0';
        logf("%04X: %s", l * 16, line);
    }
}

IAPBool iap_platform_on_acc_samprs_received(struct IAPContext* iap_ctx, struct IAPSpan* samprs) {
    (void)iap_ctx;

    bool has_44k = false;
    bool has_48k = false;
    while(samprs->size > 0) {
        uint32_t sample_rate;
        check_act(iap_span_read_32(samprs, &sample_rate), return iap_false);
        has_44k |= sample_rate == SAMPR_44;
        has_48k |= sample_rate == SAMPR_48;
    }
    check_act(has_44k && has_48k, return iap_false, "accessory lacks mandatory freq support: 44k=%d 48k=%d", has_44k, has_48k);
    check_act(mixer_switch_sink(PCM_SINK_IAP), return false);
    return iap_true;
}

uint8_t _iap_convert_play_status(int rb_audio_status) {
    if(rb_audio_status & AUDIO_STATUS_PAUSE) {
        return IAPIPodStatePlayStatus_PlaybackPaused;
    } else if(rb_audio_status & AUDIO_STATUS_PLAY) {
        return IAPIPodStatePlayStatus_Playing;
    } else {
        return IAPIPodStatePlayStatus_PlaybackStopped;
    }
}

uint8_t _iap_convert_volume(int rb_volume) {
    return normalize_8(rb_volume, sound_min(SOUND_VOLUME), sound_max(SOUND_VOLUME));
}

uint8_t _iap_convert_shuffle_state(bool rb_state) {
    return rb_state ? IAPIPodStateShuffleSettingState_Tracks : IAPIPodStateShuffleSettingState_Off;
}

uint8_t _iap_convert_repeat_state(int rb_state) {
    static const uint8_t table[] = {
        [REPEAT_OFF]     = IAPIPodStateRepeatSettingState_Off,
        [REPEAT_ALL]     = IAPIPodStateRepeatSettingState_All,
        [REPEAT_ONE]     = IAPIPodStateRepeatSettingState_One,
        [REPEAT_SHUFFLE] = IAPIPodStateRepeatSettingState_All,
#ifdef AB_REPEAT_ENABLE
        [REPEAT_AB] = IAPIPodStateRepeatSettingState_One,
#endif
    };

    if(rb_state == REPEAT_OFF && global_settings.next_folder) {
        /* when repeat is off and next folder is enabled, report repeat all.
         * without this hack, the accessory will likely set repeat to all,
         * resulting in trapping us in the current directory. */
        return IAPIPodStateRepeatSettingState_All;
    } else {
        check_act(rb_state < ARRAY_SIZE(table), return iap_false);
        return table[rb_state];
    }
}

uint8_t _iap_convert_battery_level(int rb_battery_level) {
    return 0xFF * rb_battery_level / 100;
}

uint8_t _iap_convert_charge_status(enum charge_state_type rb_charge_state) {
    switch(rb_charge_state) {
    case CHARGING:
        return IAPIPodStatePowerState_ExternalCharging;
    case TOPOFF:
    case TRICKLE:
        return IAPIPodStatePowerState_ExternalCharged;
        break;
    default:
        return IAPIPodStatePowerState_Internal;
    }
}

void _iap_convert_datetime(struct tm* rb_time, struct IAPDateTime* time) {
    time->year    = rb_time->tm_year + 1900;
    time->month   = rb_time->tm_mon + 1;
    time->day     = rb_time->tm_mday;
    time->hour    = rb_time->tm_hour;
    time->minute  = rb_time->tm_min;
    time->seconds = rb_time->tm_sec;
}
