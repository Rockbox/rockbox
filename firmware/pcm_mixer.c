/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Michael Sevakis
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
#include "config.h"
#include "system.h"
#include "general.h"
#include "kernel.h"
#include "pcm-internal.h"

#define PCM_STR_DESC_ADDR_OFFS (offsetof (struct pcm_stream_desc, addr))
#define PCM_STR_DESC_AMP_OFFS  (offsetof (struct pcm_stream_desc, amplitude))

/* Packed pointer array of all not-stopped streames (+ NULL) */
static struct pcm_stream_desc * active_streams[PCM_MAX_HANDLES+1] IBSS_ATTR;

/* Number of silence periods to play after all data has played (3 seconds) */
#define MAX_IDLE_PERIODS ((mixer_sampr*3 / PCM_DBL_BUF_FRAMES))
static unsigned long mixer_sampr = 0;
static unsigned int idle_counter = 0; /* 0 if stopped, 1 .. MAX otherwise */

/** Mixing routines, CPU optmized **/
#include "asm/pcm-mixer.c"

#define CONV_DBE(_code, _chnum, _gain, _write, _mix) \
    { .code = (PCM_FORMAT_##_code), .chnum = (_chnum), .gain = (_gain), \
      .conv = { .write = (_write), .mix = (_mix) } }

int pcm_mixer_format_config(struct pcm_stream_desc *desc)
{
    static const struct
    {
        unsigned int code  : 8; /* PCM_FORMAT_x */
        unsigned int chnum : 2; /* number of input channels */
        unsigned int gain  : 1; /* 0=unity, 1=gain */
        struct pcm_dma_t_convert conv;
    } conv_db[] =
    {
        CONV_DBE(S16_2, 1, 0, write_samples_unity_1ch_s16_2,
                              mix_samples_unity_1ch_s16_2),
        CONV_DBE(S16_2, 1, 1, write_samples_gain_1ch_s16_2,
                              mix_samples_gain_1ch_s16_2),
        CONV_DBE(S16_2, 2, 0, write_samples_unity_2ch_s16_2,
                              mix_samples_unity_2ch_s16_2),
        CONV_DBE(S16_2, 2, 1, write_samples_gain_2ch_s16_2,
                              mix_samples_gain_2ch_s16_2),
    };

    unsigned int code  = desc->format.code;
    unsigned int chnum = desc->format.chnum;
    unsigned int gain  = desc->amplitude != PCM_AMP_UNITY;

    for (unsigned int i = 0; i < ARRAYLEN(conv_db); i++) {
        typeof (*conv_db) *dbe = &conv_db[i];

        if (dbe->code == code &&
            dbe->chnum == chnum &&
            dbe->gain  == gain) {
            desc->conv = dbe->conv;
            return 0;
        }
    }

    return -1;
}

/* Make stream active to allow it to mix */
static inline void activate_stream(struct pcm_stream_desc *desc)
{
    void **elem = find_array_ptr((void **)active_streams, desc);

    if (!*elem) {
        idle_counter = MAX_IDLE_PERIODS;
        *elem = desc;
    }
}

/* Stop stream from mixing */
static inline void deactivate_stream(struct pcm_stream_desc *desc)
{
    remove_array_ptr((void **)active_streams, desc);
}

/* Deactivate stream and change it to stopped state */
static inline void stream_stopped(struct pcm_stream_desc *desc)
{
    deactivate_stream(desc);
    pcm_stream_stopped(desc);
}

/* Stop all streams and inform of error status if any */
static void stop_streams(int status, struct pcm_stream_desc *desc_except)
{
    struct pcm_stream_desc **active = active_streams;
    struct pcm_stream_desc *desc;

    while ((desc = *active)) {
        if (desc == desc_except) {
            active++;
            continue;
        }

        if (status != 0 && desc->callback) {
            desc->addr = NULL;
            desc->frames = 0;
            desc->callback(status, &desc->addr, &desc->frames);
        }

        stream_stopped(desc);
    }
}

/* Buffering callback - calls sub-callbacks and mixes the data for the
   next buffer to be sent */
unsigned long MIXER_CALLBACK_ICODE
pcm_mixer_fill_buffer(int status, void *outbuf, unsigned long frames)
{
    if (!PCM_STATUS_CONTINUABLE(status)) {
        stop_streams(status, NULL);
        return 0;
    }

    void *mixptr = outbuf;
    unsigned long framesrem = frames;
    unsigned long mixframes = frames;
    struct pcm_stream_desc **desc_p, *desc;

    /* "Loop" back here if one round wasn't enough to fill a buffer */
fill_buffer:
    desc_p = active_streams;
    desc = *desc_p;

    while (desc) {
        /* Find the active channel with the least data remaining and call any
           callbacks for channels that ran out - stopping whichever report
           "no more" */
        desc->frames -= desc->prev_frames;

        if (desc->frames) {
            desc->addr += desc->prev_frames*pcm_desc_frame_size(desc);
        }
        else {
            int st = status;

            if (desc->callback) {
                desc->addr = NULL;
                st = desc->callback(status, &desc->addr, &desc->frames);
            }

            if (st != 0 || !(desc->addr && desc->frames)) {
                stream_stopped(desc);
                desc = *desc_p;
                continue;
            }
        }

        /* Channel with the fewest frames remaining sets the count */
        if (desc->frames < mixframes) {
            mixframes = desc->frames;
        }

;       desc = *++desc_p;
    }

    /* Add all still-active channels to the downmix */
    desc_p = active_streams;
    desc = *desc_p;

    if (LIKELY(desc)) {
        /* write the first stream */
        desc->conv.write(desc, mixptr, mixframes);
        desc->prev_frames = mixframes;

        /* mix any additional streams */
        while ((desc = *++desc_p)) {
            desc->conv.mix(desc, mixptr, mixframes);
            desc->prev_frames = mixframes;
        }

        framesrem -= mixframes;

        if (framesrem) {
            /* There is still space remaining in this frame */
            mixptr += pcm_dma_t_frames2size(mixframes);
            mixframes = framesrem;
            goto fill_buffer;
        }
    }
    else if (idle_counter) {
        /* Pad incomplete periods with silence/fill idle silence period */
        idle_counter--;
        memset(mixptr, 0, pcm_dma_t_frames2size(framesrem));
    }
    else {
        /* Silence period ran out - go to sleep */
        frames = 0;
    }

#ifdef HAVE_SW_VOLUME_CONTROL
    if (frames) {
        /* Apply final volume setting (before cleanup code) */
        pcm_sw_volume_apply(outbuf, frames);
    }
#endif /* HAVE_SW_VOLUME_CONTROL */

    /* Certain SoC's have to do cleanup */
    mixer_buffer_cleanup();

    return frames;
}

/* Puts the stream on the active list */
void pcm_mixer_activate_stream(struct pcm_stream_desc *desc)
{
    activate_stream(desc);
}

/* Removes the stream from the active list */
void pcm_mixer_deactivate_stream(struct pcm_stream_desc *desc)
{
    deactivate_stream(desc);
}

/* Cancels all active streams except the specified one */
bool pcm_mixer_apply_settings(struct pcm_stream_desc *desc,
                              const struct pcm_hw_settings *settings)
{
    mixer_sampr = settings->samplerate;

    if (desc) {
        stop_streams(0, desc);
    }

    bool any = !!*active_streams;

    if (!any) {
        idle_counter = 0;
    }

    return any;
}
