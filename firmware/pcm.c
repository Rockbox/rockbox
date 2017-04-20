/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include <stdlib.h>
#include "system.h"
#include "kernel.h"

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "general.h"
#include "pcm.h"
#include "pcm-internal.h"

/**
 * Aspects implemented in the target-specific portion:
 *
 * ==Playback==
 *   Public -
 *      pcm_postinit
 *   Semi-private -
 *      pcm_play_dma_init
 *      pcm_play_dma_postinit
 *      pcm_play_dma_lock
 *      pcm_play_dma_unlock
 *      pcm_play_dma_complete_callback
 *      pcm_play_dma_start
 *      pcm_play_dma_stop
 *   Data Read/Written within TSP -
 *      pcm_sampr (RO)
 *      pcm_fsel (RO)
 *      pcm_curr_sampr (RO)
 *
 * ==Playback/Recording==
 *   Semi-private -
 *      pcm_dma_apply_settings
 *
 * ==Recording==
 *   Semi-private -
 *      pcm_rec_dma_lock
 *      pcm_rec_dma_unlock
 *      pcm_rec_dma_complete_callback
 *      pcm_rec_dma_init
 *      pcm_rec_dma_close
 *      pcm_rec_dma_start
 *      pcm_rec_dma_stop
 *      pcm_rec_dma_get_peak_buffer
 *
 */

#define IS_CAPTURE_STREAM(flags) \
    (PCM_FLAGS_STREAM_TYPE(flags) == PCM_STREAM_RECORDING)

/* dimension info on a single sample of one channel, indexed by format code */
static const struct
{
    uint8_t size;
    uint8_t depth;
} pcm_sample_dims[] =
{
    [PCM_FORMAT_S16_2] = { .size = 2, .depth = 16 },
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_S24_4)
    [PCM_FORMAT_S24_4] = { .size = 4, .depth = 24 },
#endif
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_S32_4)
    [PCM_FORMAT_S32_4] = { .size = 4, .depth = 32 },
#endif
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_S24_3)
    [PCM_FORMAT_S24_3] = { .size = 3, .depth = 24 },
#endif
};

/* 'true' when all stages of pcm initialization have completed */
static volatile bool pcm_is_ready = false;

static unsigned int pcm_play_locked = 0;    /* lock recursion counter */
#ifdef HAVE_RECORDING
static unsigned int pcm_rec_locked = 0;     /* lock recursion counter */
static struct pcm_stream_desc *cur_rec_desc = NULL; /* only one for now */
#endif

static struct pcm_stream_desc pcm_descs[PCM_MAX_HANDLES] IBSS_ATTR;
static uint32_t free_desc_mask IBSS_ATTR;

/* Wait for full init to complete */
static void pcm_wait_for_init(void)
{
    while (!pcm_is_ready) {
        sleep(0);
    }
}

/* Assign the next handle */
static void desc_next_handle(struct pcm_stream_desc *desc)
{
    unsigned long version = desc->id / PCM_MAX_HANDLES;
    unsigned long index = desc->id - version*PCM_MAX_HANDLES;

    if (++version == 0) {
        version = 1;
    }

    desc->id = version*PCM_MAX_HANDLES + index; 
}

/* Allocate a PCM handle from the pool */
static struct pcm_stream_desc * pcm_alloc_desc(void)
{
    if (!free_desc_mask) {
        return NULL;
    }

    unsigned long index = find_first_set_bit(free_desc_mask);
    free_desc_mask &= ~(1ul << index);

    return &pcm_descs[index];
}

/* Check that the handle is valid and is open */
static struct pcm_stream_desc * pcm_handle_desc(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = &pcm_descs[pcm % PCM_MAX_HANDLES];
    if (desc->id == pcm) {
        return desc;
    }

    logf("PCM:invalid handle:%lu", pcm);
    return NULL;
}

/* Return a PCM handle to the pool */
static void pcm_free_desc(struct pcm_stream_desc *desc)
{
    uint32_t bit = 1ul << (desc->id % PCM_MAX_HANDLES);
    desc_next_handle(desc);
    free_desc_mask |= bit;
}

/* Lock out the DMA interrupt */
static void lock_stream_callback(struct pcm_stream_desc *desc)
{
#ifdef HAVE_RECORDING
    if (IS_CAPTURE_STREAM(desc->flags)) {
        if (++pcm_rec_locked == 1) {
            pcm_rec_dma_lock();
        }
    }
    else
#endif
    {
        if (++pcm_play_locked == 1) {
            pcm_play_dma_lock();
        }
    }

    (void)desc;
}

/* Allow DMA interrupts */
static int unlock_stream_callback(struct pcm_stream_desc *desc)
{
#ifdef HAVE_RECORDING
    if (IS_CAPTURE_STREAM(desc->flags)) {
        if (!pcm_rec_locked) {
            return -1;
        }

        if (--pcm_rec_locked == 0) {
            pcm_rec_dma_unlock();
        }
    }
    else
#endif
    {
        if (!pcm_play_locked) {
            return -1;
        }

        if (--pcm_play_locked == 0) {
            pcm_play_dma_unlock();
        }
    }

    return 0;
    (void)desc;
}

/* Stream locks may eventually behave differently so the following are reserved
 * for when global DMA lockout is needed without restriction */

/* Globally lock out PCM interrupt */
void pcm_play_lock_device_callback(void)
{
    if (++pcm_play_locked == 1) {
        pcm_play_dma_lock();
    }
}

/* Globally reenable PCM interrupt */
void pcm_play_unlock_device_callback(void)
{
    if (--pcm_play_locked == 0) {
        pcm_play_dma_unlock();
    }
}

/**/

/* Stream has stopped playing; reset it -- called by API and for DMA event */
void pcm_stream_stopped(struct pcm_stream_desc *desc)
{
    desc->addr     = NULL;
    desc->frames   = 0;
    desc->state    = PCM_STATE_STOPPED;
    desc->callback = NULL;
}

/**
 * Perform peak calculation on a buffer of samples
 *
 * Used for recording and playback.
 */
static void pcm_peak_peeker(struct pcm_stream_desc *desc, struct pcm_peaks *peaks)
{
    const pcm_dma_t *pend = peaks->end_addr;

    for (unsigned int ch = desc->chnum; ch--;) {
        peaks->peak[ch] = 0;
    }

    switch (desc->sample_size)
    {
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_CAPS)
    case 4:
    {
        const int32_t *p = peaks->addr;

        while (p < end) {
            for (unsigned int ch = desc->chnum; ch--;) {
                uint32_t s_abs = UABS(uint32_t, *p++);

                if (s_abs > peaks->peak[ch]) {
                    peaks->peak[ch] = s_abs;
                }
            }

            p += 3*desc->chnum; /* every 4th sample only */
        }

        break;
        } /* 4 */
#endif /* CONFIG_CAPS */
#if CONFIG_PCM_FORMAT_CAPS == PCM_FORMAT_CAP_S16_2
    default:
#else
    case 2:
#endif
    {
        const int16_t *p = peaks->addr;

        while (p < end) {
            for (unsigned int ch = desc->chnum; ch--;) {
                uint32_t s_abs = UABS(uint32_t, *p++);

                if (s_abs > peaks->peak[ch]) {
                    peaks->peak[ch] = s_abs;
                }
            }

            p += 3*desc->chnum; /* every 4th sample only */
        }

        break;
        } /* 2/default */
    } /* switch */
}

/* Set peaks in all channels to zero */ 
static inline void pcm_zero_peaks(unsigned int chnum,
                                  struct pcm_peaks *peaks)
{
    while (chnum--) {
        peaks->peak[chnum] = 0;
    }

    peaks->end_addr = NULL;
}


/** PCM global init, state and settings **/

/* This should only be called at startup before any audio playback or
   recording is attempted */
void pcm_init(void)
{
    logf("pcm_init");

    /* set initial next handles for each descriptor */
    for (unsigned long i = 0; i < PCM_MAX_HANDLES; i++) {
        struct pcm_stream_desc *desc = &pcm_descs[i];
        desc->id = 1*PCM_MAX_HANDLES + i;
    }

    free_desc_mask = MASK_N(uint32_t, PCM_MAX_HANDLES, 0);

    pcm_set_frequency(HW_SAMPR_DEFAULT);

    pcm_play_dma_init();
}

/* Finish delayed init */
void pcm_postinit(void)
{
    logf("pcm_postinit");
    pcm_play_dma_postinit();
    pcm_is_ready = true;
}

/* Return 'true' if all PCM init has completed */
bool pcm_is_initialized(void)
{
    return pcm_is_ready;
}

/* Stop everything and reset state */
void pcm_reset(void)
{
    if (++pcm_play_locked == 1) {
        pcm_play_dma_lock();
    }

    pcm_playback_stop_dma();
    pcm_mixer_reset();

    if (--pcm_play_locked == 0) {
        pcm_play_dma_unlock();
    }

#ifdef HAVE_RECORDING
    if (++pcm_rec_locked == 1) {
        pcm_rec_dma_lock();
    }

    if (cur_rec_desc) {
        pcm_recording_stop_capture(cur_rec_desc);
    }

    if (--pcm_rec_locked == 0) {
        pcm_rec_dma_unlock();
    }
#endif
}

/* set frequency next frequency used by the audio hardware -
 * what pcm_apply_settings will set */
unsigned long pcm_set_frequency(unsigned long samplerate)
{
    logf("pcm_set_frequency:sr=%lu", samplerate);

    int index;

#ifdef CONFIG_SAMPR_TYPES
    unsigned int type = samplerate & SAMPR_TYPE_MASK;
    samplerate &= ~SAMPR_TYPE_MASK;

    /* For now, supported targets have direct conversion when configured with
     * CONFIG_SAMPR_TYPES.
     * Some hypothetical target with independent rates would need slightly
     * different handling throughout this source. */
    samplerate = pcm_sampr_to_hw_sampr(samplerate, type);
#endif /* CONFIG_SAMPR_TYPES */

    index = round_value_to_list32(samplerate, hw_freq_sampr,
                                  HW_NUM_FREQ, false);

    if (samplerate != hw_freq_sampr[index])
        index = HW_FREQ_DEFAULT; /* Invalid = default */

    __pcm_sampr = hw_freq_sampr[index];
    __pcm_fsel = index;

    return __pcm_sampr;
}

/* return last-set frequency */
unsigned long pcm_get_frequency(void)
{
    return __pcm_sampr;
}

/* apply pcm settings to the hardware */
void pcm_apply_settings(void)
{
    logf("pcm_apply_settings");

    pcm_wait_for_init();

    if (__pcm_sampr != __pcm_curr_sampr)
    {
        logf(" pcm_dma_apply_settings");
        pcm_dma_apply_settings();
        __pcm_curr_sampr = __pcm_sampr;
    }
}


/** PCM Playback **/
static pcm_dma_t pcm_dbl_buf[2][PCM_DBL_BUF_FRAMES*PCM_DMA_T_CHANNELS]
        PCM_DBL_BUF_IBSS MEM_ALIGN_ATTR;
static unsigned long pcm_dbl_buf_frames[2] IBSS_ATTR;
static int pcm_dbl_buf_index = -1;   /* Which pcm_dbl_buf? (-1 == not playing) */

/* samplerate of currently playing audio - undefined if stopped */
unsigned long __pcm_curr_sampr SHAREDBSS_ATTR = 0;
/* samplerate waiting to be set */
unsigned long __pcm_sampr SHAREDBSS_ATTR = HW_SAMPR_DEFAULT;
/* samplerate frequency selection index */
int __pcm_fsel SHAREDBSS_ATTR = HW_FREQ_DEFAULT;

/* Vet PCM format and setup dependent parts */
static int setup_playback_format(struct pcm_stream_desc *desc,
                                 const struct pcm_format *format)
{
    desc->format = *(format ?: PCM_FORMAT_DMA_T());

    int rc = pcm_mixer_format_config(desc)
    if (rc == 0) {
        desc->sample_size  = pcm_sample_dims[format->code].size;
        desc->frame_size   = desc->sample_size * format->chnum;
        desc->sample_depth = pcm_sample_dims[format->code].depth;
    }
    else {
        logf("PCM:bad format:code=%u:chnum=%u", format->code, format->chnum);
    }

    return rc;
}

/* Stop the playback DMA transfer */
static inline void pcm_playback_stop_dma(void)
{
    pcm_play_dma_stop();
    pcm_dbl_buf_index = -1;
}

/* Start the playback DMA transfer if not already going */
static void pcm_playback_start_dma(struct pcm_stream_desc *desc)
{
    pcm_mixer_activate_stream(desc);

    if (pcm_dbl_buf_idx >= 0) {
        return;
    }

    /* PCM driver not running */
    memset(pcm_dbl_buf, 0, sizeof (pcm_dbl_buf));
    pcm_dbl_buf_idx = 0;

    /* Two shortened silence frames to start cycle (1 frames latency total).
       First mixed frame will be done in parallel with 2nd buffer so that
       should be as long as possible */
    pcm_dbl_buf_frames[0] = 32; /* enough to top off any hw fifo */
    pcm_dbl_buf_frames[1] = PCM_DBL_BUF_FRAMES - 32;

#ifdef HAVE_SW_VOLUME_CONTROL
    /* Smoothed transition might not have happened so sync now */
    pcm_sw_volume_sync_factors();
#endif
    pcm_apply_settings();
    pcm_play_dma_prepare();
    pcm_play_dma_send_frames(pcm_dbl_buf[0], pcm_dbl_buf_frames[0]);
}

/* Pause or resume a playback stream */
static void pcm_playback_pause(struct pcm_stream_desc *desc,
                               bool pause)
{
    if (pause) {
        pcm_mixer_deactivate_stream(desc);
    }
    else {
        pcm_playback_start_dma(desc);
    }
}

/* Stop the stream playing and reset it */
static void pcm_playback_stop(struct pcm_stream_desc *desc)
{
    pcm_mixer_deactivate_stream(desc);
    pcm_stream_stopped(desc);
}

/* Handle playback-specific tasks when closing a stream */
static void pcm_playback_close(struct pcm_stream_desc *desc)
{
    if (desc->state != PCM_PLAYBACK_STOPPED) {
        pcm_playback_stop(desc);
    }
}

/* Get number of frames remaining to be sent for playback */
static const void * pcm_play_get_remaining_frames(struct pcm_stream_desc *desc,
                                                  unsigned long *frames_rem)
{
    const void *addr = *(const void * volatile *)&desc->addr;
    unsigned long rem = *(unsigned long volatile *)&desc->frames;
    const void *addr2 = *(const void * volatile *)&desc->addr;

    /* Still same buffer? */
    if (addr != addr2) {
        rem = 0;
        addr = NULL;
    }

    *frames_rem = rem;
    return addr;
}

/* Get peaks for the currently playing buffer */
static void pcm_play_get_peaks(struct pcm_stream_desc *desc,
                               struct pcm_peaks *peaks)
{
    long tick = current_tick;
    /* Peak no farther ahead than expected period to avoid overcalculation */
    long period = tick - peaks->tick;

    peaks->addr = pcm_play_get_remaining_frames(pcm, &peaks->frames);

    /* Keep reasonable limits on period */
    if (period < 1) {
        period = 1;
    }
    else if (period > HZ/5) {
        period = HZ/5;
    }

    peaks->period = (3*peaks->period + period) / 4;
    peaks->tick = tick;

    if (pcm->state == PCM_STATE_RUNNING) {
        unsigned long lookahead = peaks->period*__pcm_curr_sampr / HZ;
        peaks->frames = MIN(lookahead, peaks->frames);

        if (peaks->addr && peaks->frames) {
            peaks->end_addr = peaks->addr + peaks->frames*desc->frame_size;
            pcm_peak_peeker(pcm, peaks);
        }
        /* else keep previous peak values */

        return;
    }

    pcm_zero_peaks(pcm->chnum, peaks);
}

/* Called back by driver when last buffer has finished or an error occurs */
void pcm_play_dma_complete_callback(int status)
{
    int index0 = pcm_dbl_buf_index;
    int index1 = 1 - index0;

    pcm_dbl_buf_index = index1;

    unsigned long frames = pcm_play_dbl_buf_frames[index0];
    pcm_play_dbl_buf_frames[index0] = 0;

    if (status == 0 && frames) {
        pcm_play_dma_send_frames(pcm_play_dbl_buf[index0], frames);

        pcm_play_dbl_buf_frames[index1] =
            pcm_mixer_fill_buffer(pcm_play_dbl_buf[index1], PCM_DBL_BUF_FRAMES);
        return;
    }

    /* Error or no more DMA to do */
    pcm_playback_stop_dma();
}

/* Play some audio */
void pcm_play_data(pcm_handle_t pcm,
                   pcm_play_callback_type callback,
                   const void *start,
                   unsigned long frames,
                   const struct sample_format *format)
{
    logf("pcm_play_data");

    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

#ifdef HAVE_RECORDING
    if (IS_CAPTURE_STREAM(desc->flags)) {
        logf("  can't play capture stream");
        return -1;
    }
#endif /* HAVE_RECORDING */

    lock_stream_callback(desc);

    int rc = setup_playback_format(desc, format);

    if (rc == 0) {
        /* if there's a callback, mixer will call it to get first buffer
           else it will just mix from the supplied buffer */
        if (!(addr && frames)) {
            addr = NULL;
            frames = 0;
        }

        desc->start       = start;
        desc->frames      = frames;
        desc->prev_frames = 0;
        desc->callback    = callback;
        desc->state       = PCM_STATE_RUNNING;

        pcm_playback_start_dma(desc);
    }

    unlock_stream_callback(desc);

    return rc;
}

/** PCM Capture **/
#ifdef HAVE_RECORDING

/* Stops the capture and resets the stream */
static void pcm_recording_stop_capture(struct pcm_stream_desc *desc)
{
    pcm_rec_dma_stop();
    pcm_stream_stopped(desc);
}

/* Vet PCM format and setup dependent parts */
static int setup_record_format(struct pcm_stream_desc *desc,
                               const struct pcm_format *format)
{
    desc->format = *(format ?: PCM_FORMAT_DMA_T());

    /* TODO: work out mono, format compatibility and other stuff */
    if (format->code != PCM_FORMAT_S16_2 || format->chnum != 2) {
        logf("PCM:bad format:code=%u:chnum=%u", format->code, format->chnum);
        return -1;
    }
    else {
        desc->sample_size  = pcm_sample_dims[PCM_FORMAT_S16_2].size;
        desc->frame_size   = desc->sample_size * 2;
        desc->sample_depth = pcm_sample_dims[PCM_FORMAT_S16_2].depth;
        return 0;
    }
}

/* Start the driver capturing audio */
static void pcm_record_start_dma(struct pcm_stream_desc *desc)
{
    desc->seqnum_rec++;
    pcm_apply_settings();
    pcm_rec_dma_capture_frames_prepare();
    pcm_rec_dma_capture_frames(desc->addr_rec, desc->frames);
}

/**
 * Return recording peaks - From the end of the last peak up to
 *                          current write position.
 */
static void pcm_recording_get_peaks(struct pcm_stream_desc *desc,
                                    struct pcm_peaks *peaks)
{
    if (desc->state == PCM_STATE_RUNNING) {
        peaks->addr = pcm_rec_dma_get_peak_buffer(&peaks->frames);

        if (peaks->addr && peaks->frames) {
            unsigned long seq = desc->seqnum_rec;
            const void *end_addr = peaks->end_addr;
            const void *end = peaks->addr + peaks->frames*desc->frame_size;

            if (peaks->seq == seq && end_addr >= peaks->addr && end_addr <= end) {
                /* carry on from previous endpoint */
                peaks->addr = end_addr;
            }

            peaks->end_addr = end;
            peaks->seq = seq;

            if (peaks->addr < end) {
                pcm_peak_peeker(desc, peaks);
            }
        }

        /* else keep previous peak values */
        return;
    }

    pcm_zero_peaks(desc->chnum, peaks);
}

/* Called back by driver when last buffer is ready or an error occurs */
void pcm_rec_dma_complete_callback(int status)
{
    struct pcm_stream_desc *desc = cur_rec_desc;
    pcm_rec_callback_type callback = desc->callback_rec;

    if (desc->state == PCM_STATE_RUNNING) {
        desc->addr_rec = NULL;
        desc->frames = 0;

        if (callback) {
            status = callback(status, &desc->addr_rec, &desc->frames);
            ALIGN_AUDIOBUF(desc->addr_rec, desc->frames, 2*2);
        }
    }
    else {
        /* recording DMA still runs when paused in order to keep level
           monitoring active - just use the same buffer again */
        if (PCM_ERR_RECOVERABLE(status)) {
            status = 0; /* ignore when paused */
        }
    }

    if (status == 0 && desc->addr_rec && desc->frames) {
        desc->seqnum_rec++;
        pcm_rec_dma_capture_frames(desc->addr_rec, desc->frames);
    }
    else {
        /* Error, callback missing or no more DMA to do */
        pcm_recording_stop_capture(desc);
    }
}

/* Initialize the recording part of the API */
static int pcm_recording_init(struct pcm_stream_desc *desc)
{
    logf("pcm_recording_init");

    if (cur_rec_desc) {
        logf(" already initialized");
        return -1;
    }

    pcm_rec_dma_init();
    cur_rec_desc = desc;

    return 0;
}

/* Close the recording part of the API */
static void pcm_recording_close(struct pcm_stream_desc *desc)
{
    if (desc->state != PCM_STATE_STOPPED) {
        pcm_recording_stop_capture(desc);
    }

    pcm_rec_dma_close();
    cur_rec_desc = NULL;
}

/* Stop the capture and reset the stream */
static void pcm_recording_stop(struct pcm_stream_desc *desc)
{
    pcm_recording_stop_capture(desc);
}

/* Record some audio */
int pcm_record_data(pcm_handle_t pcm,
                    pcm_record_callback_type callback,
                    void *addr,
                    unsigned long frames,
                    const struct pcm_format *format)
{
    logf("pcm_record_data");

    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    if (!IS_CAPTURE_STREAM(desc->flags)) {
        logf("  can't record playback stream");
        return -1;
    }

    lock_stream_callback(desc);

    if (desc->state != PCM_STATE_STOPPED) {
        pcm_recording_stop_capture(desc);
    }

    int rc = setup_record_format(desc, format);

    if (rc == 0) {
        ALIGN_AUDIOBUF(addr, frames, 2*2);

        if (addr && frames) {
            desc->addr_rec     = addr;
            desc->frames       = frames;
            desc->callback_rec = callback;
            pcm_record_start_dma(desc);
            desc->state = PCM_STATE_RUNNING;
        }
        else {
            logf(" no buffer");
        }
    }

    unlock_stream_callback(desc);

    return 0;
}

#endif /* HAVE_RECORDING */

/* Open a handle to a PCM stream */
int pcm_open(pcm_handle_t *pcm_out, unsigned int flags)
{
    if (!pcm_out) {
        return -1;
    }

    *pcm_out = 0;

    pcm_wait_for_init();

    struct pcm_stream_desc *desc = pcm_alloc_desc();
    if (!desc) {
        return -1;
    }

    int rc = 0;

    desc->flags       = flags;
    desc->start       = NULL;
    desc->frames      = 0;
    desc->amplitude   = PCM_AMP_UNITY;
    desc->prev_frames = 0;
    desc->callback    = NULL;
    desc->state       = PCM_STATE_STOPPED;

    if (IS_CAPTURE_STREAM(flags)) {
    #ifdef HAVE_RECORDING
        lock_stream_callback(desc);
        rc = pcm_recording_init(desc);
        unlock_stream_callback(desc);
    #else /* ndef HAVE_RECORDING */
        rc = -1;
    #endif /* HAVE_RECORDING */
    }

    if (rc) {
        pcm_free_desc(desc);
    }
    else {
        *pcm_out = desc->id;
    }

    return rc;
}

/* Close a stream and free its resources */
int pcm_close(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    lock_stream_callback(desc);

#ifdef HAVE_RECORDING
    if (IS_CAPTURE_STREAM(desc->flags)) {
        pcm_recording_close(desc);
    }
    else
#endif /* HAVE_RECORDING */
    {
        pcm_playback_close(desc);
    }

    /* state set to 'stopped' by above call */

    unlock_stream_callback(desc);

    pcm_free_handle(desc);

    return 0;
}

/* Lock out the stream's callback */
int pcm_lock_callback(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    lock_stream_callback(desc);

    return 0;
}

/* Release the stream's callback lockout */
int pcm_unlock_callback(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    return unlock_stream_callback(desc);
}

/* Get the current stream state */
int pcm_get_state(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    return desc->state;
}

/* Pause or resume playback or recording */
int pcm_pause(pcm_handle_t pcm, bool pause)
{
    logf("pcm_pause:pause=%d", (int)pause);

    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    lock_stream_callback(desc);

    int state = desc->state;
    int newstate = pause ? PCM_STATE_PAUSED : PCM_STATE_RUNNING;

    if (state != PCM_STATE_STOPPED && state != newstate) {
#ifdef HAVE_RECORDING
        if (!IS_CAPTURE_STREAM(desc->flags))
#endif /* HAVE_RECORDING */
        {
            pcm_playback_pause(desc, pause);
        }

        desc->state = newstate;
    }

    unlock_stream_callback(desc);

    return 0;
}

/* Stop playback or recording */
int pcm_stop(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    lock_stream_callback(desc);

    if (desc->state != PCM_STATE_STOPPED) {
#ifdef HAVE_RECORDING
        if (IS_CAPTURE_STREAM(desc->flags)) {
            pcm_recording_stop(desc);
        }
        else
#endif /* HAVE_RECORDING */
        {
            pcm_playback_stop(desc);
        }

        /* stopped state set by above call */
    }

    unlock_stream_callback(desc);

    return 0;
}

/* Set stream's amplitude factor (no effect on capture streams) */
int pcm_set_amplitude(pcm_handle_t pcm, int32_t amplitude)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    int32_t oldamp = desc->amplitude;

    if (amplitude < PCM_AMP_MIN) {
        amplitude = PCM_AMP_MIN;
    }
    else if (amplitude > PCM_AMP_MAX) {
        amplitude = PCM_AMP_MAX;
    }

    desc->amplitude = amplitude;

    if ((oldamp == PCM_AMP_UNITY) != (amplitude == PCM_AMP_UNITY)) {
        pcm_mixer_format_config(desc);
    }

    return 0;
}

/* Returns amount data remaining in channel before next callback */
unsigned long pcm_get_frames_waiting(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

    return desc->frames;
}

/* Return pointer to channel's playing audio data and the frames remaining */
const void * pcm_get_playing_buffer(pcm_handle_t pcm,
                                    unsigned long *frames_rem)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        *frames_rem = 0;
        return NULL;
    }

    return pcm_play_get_remaining_frames(desc, frames_rem);
}

/* Calculate peak values for stream */
int pcm_get_peaks(pcm_handle_t pcm, struct pcm_peaks *peaks)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

#ifdef HAVE_RECORDING
    if (IS_CAPTURE_STREAM(desc->flags)) {
        pcm_recording_get_peaks(desc, peaks);
    }
    else
#endif /* HAVE_RECORDING */
    {
        pcm_play_get_peaks(desc, peaks);
    }

    return desc->sample_depth;
}

/* Adjust channel pointer by a given offset to support movable buffers */
int pcm_adjust_address(pcm_handle_t pcm, off_t offset)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return -1;
    }

#ifdef HAVE_RECORDING
    if (IS_CAPTURE_STREAM(desc->flags)) {
        /* recording buffers can't be movable at this time since they're being
           written directly by the driver */
        return -1;
    }
#endif /* HAVE_RECORDING */

    lock_stream_callback(desc);
    desc->addr += offset;
    unlock_stream_callback(desc);

    return 0;
}
