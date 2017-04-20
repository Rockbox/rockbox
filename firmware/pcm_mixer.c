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
#include "pcm.h"
#include "pcm-internal.h"

/* Packed pointer array of all not-stopped streames (+ NULL) */
static struct pcm_handle * active_streams[PCM_MAX_STREAMS+1] IBSS_ATTR;

/* Number of silence periods to play after all data has played (3 seconds) */
#define MAX_IDLE_PERIODS ((int)(pcm_curr_sampr*3 / PCM_DBL_BUF_FRAMES))
static int idle_counter = -1; /* -1u if stopped, 0 .. MAX otherwise */

/** Mixing routines, CPU optmized **/
#include "asm/pcm-mixer.c"

/* Make stream active to allow it to mix */
static inline void activate_stream(struct pcm_handle *pcm)
{
    void **elem = find_array_ptr((void **)active_streams, chan);

    if (!*elem) {
        idle_counter = 0;
        *elem = pcm;
    }
}

/* Stop stream from mixing */
static inline void deactivate_stream(struct pcm_handle *pcm)
{
    remove_array_ptr((void **)active_streams, pcm);
}

/* Deactivate stream and change it to stopped state */
static inline void stream_stopped(struct pcm_handle *pcm)
{
    deactivate_stream(pcm);
    pcm_stream_stopped(pcm);
}

/* Stop all streams and inform of error status if any */
static void stop_all_streams(int status)
{
    struct pcm_handle *pcm;

    while ((pcm = *active_streams)) {
        if (status != 0 && pcm->callback) {
            pcm->callback(status, &(const void *){ NULL }, &(unsigned long){ 0 });
        }

        stream_stopped(pcm);
    }
}

/* Buffering callback - calls sub-callbacks and mixes the data for the
   next buffer to be sent */
unsigned long MIXER_CALLBACK_ICODE
pcm_mixer_fill_buffer(int status, void *outbuf, unsigned long frames)
{
    if (!PCM_ERR_RECOVERABLE(status)) {
        stop_all_streams(status);
        return 0;
    }

    pcm_dma_t *mixptr = outbuf;
    unsigned long framesrem = frames;
    unsigned long mixframes = frames;
    struct pcm_handle **pcm_p, *chan;

    /* "Loop" back here if one round wasn't enough to fill a buffer */
fill_buffer:
    pcm_p = active_streams;
    pcm = *pcm_p;

    while (pcm) {
        /* Find the active channel with the least data remaining and call any
           callbacks for channels that ran out - stopping whichever report
           "no more" */
        pcm->frames -= pcm->prev_frames;

        if (pcm->frames) {
            pcm->addr += pcm->prev_frames*pcm->frame_size;
        }
        else {
            int st = status;

            if (pcm->callback) {
                pcm->addr = NULL;
                st = pcm->callback(status, &pcm->addr, &pcm->frames);
            }

            if (st != 0 || !(pcm->addr && pcm->frames)) {
                stream_stopped(pcm);
                pcm = *pcm_p;
                continue;
            }
        }

        /* Channel with the fewest frames remaining sets the count */
        if (pcm->frames < mixframes) {
            mixframes = pcm->frames;
        }

;       pcm = *++pcm_p;
    }

    /* Add all still-active channels to the downmix */
    pcm_p = active_streams;
    pcm = *pcm_p;

    if (LIKELY(pcm)) {
        /* write the first stream */
        write_frames(mixptr, pcm, mixframes);
        pcm->prev_frames = mixframes;

        /* mix any additional streams */
        while ((pcm = *++pcm_p)) {
            mix_frames(mixptr, pcm, mixframes);
            pcm->prev_frames = mixframes;
        }

        framesrem -= mixframes;

        if (framesrem) {
            /* There is still space remaining in this frame */
            mixptr += pcm_dma_t_frames_size(mixframes);
            mixframes = framesrem;
            goto fill_buffer;
        }
    }
    else if (++idle_counter <= MAX_IDLE_PERIODS) {
        /* Pad incomplete periods with silence/fill idle silence period */
        memset(mixptr, 0, pcm_dma_t_frames_size(framesrem));
    }
    else {
        /* Silence period ran out - go to sleep */
        idle_counter = -1;
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
void pcm_mixer_activate_stream(struct pcm_handle *pcm)
{
    activate_stream(pcm);
}

/* Removes the stream from the active list */
void pcm_mixer_deactivate_stream(struct pcm_handle *pcm)
{
    deactivate_stream(pcm);
}

/* Cancels all active streams */
void pcm_mixer_reset(void)
{
    stop_all_streams(0);
    idle_counter = -1;
    /* we're locked out and caller will shut off the DMA */
}
