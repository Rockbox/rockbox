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
#include "playback.h"

#include "iap-usb.h"
#include "libiap/iap.h"
#include "platform.h"

#include "macros.h"

extern bool iap_initialized;

void iap_on_track_time_position(uint32_t pos_ms) {
    if(!iap_initialized) {
        return;
    }
    struct IAPContext* ctx = _iap_acquire_ctx(false);
    iap_notify_track_time_position(ctx, pos_ms);
}

void iap_on_track_playback_index(uint32_t index, bool track_ready) {
    if(!iap_initialized) {
        return;
    }

    struct IAPContext* ctx = _iap_acquire_ctx(false);

    if(track_ready) {
        /* called from from audio_finish_load_track() */
        goto notify;
    }

    /* called from audio_playlist_track_change() */
    struct Platform* plt = ctx->platform;
    if(plt->aa_slot < 0) {
        goto notify;
    }
    if(playback_current_aa_hid(plt->aa_slot) >= 0) {
        /* artwork ready, maybe preloaded track */
        goto notify;
    } else {
        /* artwork not ready, maybe after a playlist jump.
         * in this case, we will be called again from audio_finish_load_track(),
         * with track_ready == true. */
        return;
    }
notify:
    iap_notify_track_playback_index(ctx, index);
}

void iap_on_tracks_count(uint32_t count) {
    if(!iap_initialized) {
        return;
    }

    struct IAPContext* ctx = _iap_acquire_ctx(false);
    iap_notify_tracks_count(ctx, count);
}

void iap_on_play_status(int status) {
    if(!iap_initialized) {
        return;
    }

    struct IAPContext* ctx = _iap_acquire_ctx(false);
    iap_notify_play_status(ctx, _iap_convert_play_status(status));
}

void iap_on_volume(int volume) {
    if(!iap_initialized) {
        return;
    }

    struct IAPContext* ctx = _iap_acquire_ctx(false);
    iap_notify_volume(ctx, _iap_convert_volume(volume), iap_false);
}

void iap_on_shuffle_state(bool state) {
    if(!iap_initialized) {
        return;
    }

    struct IAPContext* ctx = _iap_acquire_ctx(false);
    iap_notify_shuffle_state(ctx, _iap_convert_shuffle_state(state));
}

void iap_on_repeat_state(int state) {
    if(!iap_initialized) {
        return;
    }

    struct IAPContext* ctx = _iap_acquire_ctx(false);
    iap_notify_repeat_state(ctx, _iap_convert_repeat_state(state));
}
