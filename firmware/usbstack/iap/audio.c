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
#include "core_alloc.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "pcm_sink.h"
#include "system.h"
#include "usb_drv.h"

#include "../usb_iap.h"
#include "buffer.h"
#include "libiap/iap.h"
#include "macros.h"
#include "platform.h"

static const unsigned long samprs[] = {
    SAMPR_48,
    SAMPR_44,
};

struct StagingBuffer {
    struct IAPAllocResult buf;
    uint16_t              cursor;
};
static struct StagingBuffer staging_buffers[USB_BATCH_SLOTS + 1]; /* +1 for silent buffer */
static int                  staging_buffer_index;

#define zero_buffer (staging_buffers[USB_BATCH_SLOTS])

static const uint8_t* pulled_buf;
static size_t         pulled_buf_size;
static size_t         pulled_buf_cursor;
static int8_t         set_freq; /* requested freq from rockbox */
static int8_t         cur_freq; /* requested freq from accessory */
static uint8_t        packet_count;

static bool enabled;
static bool exhausted;
static bool track_attrs_sent;

extern struct pcm_sink iap_pcm_sink;

static void sink_set_freq(uint16_t freq) {
    LOG("freq=%d", freq);

    track_attrs_sent = true;

    set_freq = freq;

    struct IAPContext* ctx = _iap_acquire_ctx(true);
    check_act(iap_select_sampr(ctx, samprs[freq]), );
    _iap_release_ctx();
}

static size_t calc_packet_size(uint8_t cur_sampr, uint8_t packet_count) {
    /* packet size calculation
     * (4 = sizeof(int16_t) * channels)
     * (1000 = usb frames per second)
     * 48000Hz:
     *   48000 * 4 / 1000 = 192.0
     *   => 192 + ...
     * 44100Hz:
     *   44100 * 4 / 1000 = 176.4
     *   => (9 * 176 + 180) + (...
     */
    if(cur_sampr == 0) {
        /*48k*/
        return 192;
    } else {
        /*44.1k*/
        return packet_count % 10 == 0 ? 180 : 176;
    }
}

static void batch_get_more(const void** ptr, size_t* len) {
    const size_t packet_size = calc_packet_size(cur_freq, packet_count);
#if 0
    const int cur_frame = usb_drv_get_frame_number();
    LOG("ex=%d set=%d cur=%d", exhausted, set_freq, cur_freq);
#endif

start:
    if(exhausted || cur_freq != set_freq) {
        *ptr = zero_buffer.buf.ptr;
        *len = packet_size;
        packet_count += 1;
        return;
    }

    if(pulled_buf_cursor == pulled_buf_size) {
        /* run out of previously pulled data.
         * let's start filling them, by requesting new data from upstream */
        if(!pcm_play_dma_complete_callback(PCM_DMAST_OK, (const void**)&pulled_buf, &pulled_buf_size)) {
            /* no more data, but we have to keep sending something as long as the audio stream interface is enabled */
            exhausted = true;
            goto start;
        }

        /* pushing_{buf,buf_size} are filled. reset cursor and continue filling */
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        pulled_buf_cursor = 0;
    }

    /* fill this single packet */
    struct StagingBuffer* stage = &staging_buffers[staging_buffer_index];
    const size_t          copy  = MIN(packet_size - stage->cursor, pulled_buf_size - pulled_buf_cursor);
    memcpy(stage->buf.ptr + stage->cursor, pulled_buf + pulled_buf_cursor, copy);
    pulled_buf_cursor += copy;
    stage->cursor += copy;
#define AUDIO_STAT 0
#if AUDIO_STAT == 1
    static int last_hz;
    static int sample;
#endif
    if(stage->cursor == packet_size) {
        *ptr                 = stage->buf.ptr;
        *len                 = stage->cursor;
        stage->cursor        = 0;
        staging_buffer_index = (staging_buffer_index + 1) % USB_BATCH_SLOTS;
        packet_count += 1;
#if AUDIO_STAT == 1
        sample += packet_size / 4;
        if(current_tick >= last_hz + HZ) {
            logf("pushed %d %d", packet_size, sample);
            sample  = 0;
            last_hz = current_tick;
        }
#endif
    } else { /* pushing_buf_cursor == pushing_buf_size */
        goto start;
    }
}

static void sink_play(const void* addr, size_t size) {
    LOG("play");

    pulled_buf        = addr;
    pulled_buf_size   = size;
    pulled_buf_cursor = 0;
    exhausted         = false;

    /* resolve pending play request before sending TrackNewAudioAttributes (by sink_set_freq).
     * some accessories will get confused when receiving it while waiting for an ack */
    struct IAPContext* ctx = _iap_acquire_ctx(true);
    struct Platform*   plt = ctx->platform;
    if(plt->control_pending) {
        check_act(iap_control_response(ctx, plt->pending_control, true), );
        plt->control_pending = false;
    }
    _iap_release_ctx();

    /* rockbox only calls set_freq when changes occur, but we must call
     * set_freq(and iap_select_sampr) at least once per connection. */
    if(!track_attrs_sent) {
        sink_set_freq(iap_pcm_sink.configured_freq);
    }
}

static void sink_stop(void) {
    LOG("stop");
    /* we don't call usb_drv_batch_stop() here,
     * because we need to send something, even not playing. */
}

static void sink_nop(void) {
}

struct pcm_sink iap_pcm_sink = {
    .caps = {
        .samprs       = samprs,
        .num_samprs   = ARRAYLEN(samprs),
        .default_freq = 0,
    },
    .ops = {
        .init     = sink_nop,
        .postinit = sink_nop,
        .set_freq = sink_set_freq,
        .lock     = sink_nop,
        .unlock   = sink_nop,
        .play     = sink_play,
        .stop     = sink_stop,
    },
};

bool iap_audio_init(void) {
    check_act(usb_drv_batch_init(AS_EP_IN, batch_get_more) == 0, return false);

    for(size_t i = 0; i < ARRAYLEN(staging_buffers); i += 1) {
        check_act(iap_alloc_usb_send_buffer(AS_PACKET_SIZE, &staging_buffers[i].buf), goto error);
        staging_buffers[i].cursor = 0;
    }
    staging_buffer_index = 0;

    memset(zero_buffer.buf.ptr, 0, AS_PACKET_SIZE);

    set_freq         = -1;
    cur_freq         = -2; /* something <0 && !=set_freq */
    enabled          = false;
    exhausted        = true;
    track_attrs_sent = false;
    packet_count     = 0;

    return true;

error:
    for(size_t i = 0; i < ARRAYLEN(staging_buffers); i += 1) {
        if(staging_buffers[i].buf.ptr != NULL) {
            core_free(staging_buffers[i].buf.handle);
            staging_buffers[i].buf.ptr = NULL;
        }
    }
    return false;
}

bool iap_audio_deinit(void) {
    check_act(usb_drv_batch_deinit() == 0, );
    for(size_t i = 0; i < ARRAYLEN(staging_buffers); i += 1) {
        core_free(staging_buffers[i].buf.handle);
        staging_buffers[i].buf.ptr = NULL;
    }
    return true;
}

bool iap_audio_enable(void) {
    LOG("enabled=%d", enabled);
    check_act(enabled || usb_drv_batch_start() == 0, return false);
    enabled = true;
    return true;
}

bool iap_audio_disable(void) {
    LOG("enabled=%d", enabled);
    check_act(!enabled || usb_drv_batch_stop() == 0, return false);
    enabled = false;
    return true;
}

bool iap_audio_set_sampr(uint32_t sampr) {
    uint16_t freq = 0;
    for(; freq < ARRAYLEN(samprs); freq += 1) {
        if(samprs[freq] == sampr) {
            break;
        }
    }
    check_act(freq < ARRAYLEN(samprs), return false);

    if(set_freq >= 0 && freq != set_freq) {
        /* Accessories should only set the frequency we requested via
         * TrackNewAudioAttributes, but in some cases, USB control requests may arrive late,
         * breaking established cur_freq == set_freq.
         * Ignoring sets of frequencies that were not requested can work around this. */
        ERROR("wrong frequency set: expected=%d got=%d", set_freq, freq);
        return true;
    }

    cur_freq = freq;

    LOG("sampr=%lu, set_freq=%d cur_freq=%d", sampr, set_freq, cur_freq);

    return true;
}
