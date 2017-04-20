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
/* #define LOGF_ENABLE */
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "general.h"
#include "pcm-internal.h"

/**
 * Aspects implemented in the target-specific portion:
 *
 * ==Playback/Recording==
 *   Public -
 *      pcm_init
 *   Semi-private
 *      pcm_dma_apply_settings
 *
 * ==Playback==
 *   Semi-private -
 *      pcm_play_dma_init
 *      pcm_play_dma_postinit
 *      pcm_play_dma_lock
 *      pcm_play_dma_unlock
 *      pcm_play_dma_complete_callback
 *      pcm_play_dma_start
 *      pcm_play_dma_stop
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
static volatile bool pcm_is_ready SHAREDBSS_ATTR = false;
static struct pcm_hw_settings hw_settings SHAREDDATA_ATTR =
{
    .samplerate = HW_SAMPR_DEFAULT,
    .fsel       = HW_FREQ_DEFAULT,
};

static struct pcm_stream_desc pcm_descs[PCM_MAX_HANDLES] IBSS_ATTR;
static uint32_t free_desc_mask IBSS_ATTR;

/** PCM playback data **/
static unsigned int pcm_play_locked SHAREDBSS_ATTR = 0; /* lock recursion counter */

static pcm_dma_t pcm_play_dbl_buf[2][PCM_DBL_BUF_FRAMES*PCM_DMA_T_CHANNELS]
        PCM_DBL_BUF_IBSS MEM_ALIGN_ATTR;
static unsigned long pcm_play_dbl_buf_frames[2] IBSS_ATTR;
static int pcm_play_dbl_buf_index IDATA_ATTR = -1; /* Which pcm_dbl_buf? (-1 == not playing) */

static void copy_pcm_format(struct pcm_stream_desc *desc,
                            const struct pcm_format *format)
{
    desc->format = format ? *format: *PCM_FORMAT_DMA_T();
}

/* Stop the playback DMA transfer */
static inline void pcm_playback_stop_dma(void)
{
    pcm_play_dma_stop();
    pcm_play_dbl_buf_index = -1;
}

/** PCM capture data **/
#ifdef HAVE_RECORDING
static unsigned int pcm_rec_locked SHAREDBSS_ATTR = 0; /* lock recursion counter */
static struct pcm_stream_desc *cur_rec_desc SHAREDBSS_ATTR = NULL; /* only one for now */
#endif

/* Wait for full init to complete */
static void pcm_wait_for_init(void)
{
    while (!pcm_is_ready) {
        sleep(0);
    }
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
    if (pcm >= PCM_MAX_HANDLES) {
         struct pcm_stream_desc *desc = &pcm_descs[pcm % PCM_MAX_HANDLES];
        if (desc->id == pcm) {
            return desc;
        }
    }

    logf("PCM:invalid handle:%lu", pcm);
    return NULL;
}

/* Return a PCM handle to the pool */
static void pcm_free_desc(struct pcm_stream_desc *desc)
{
    unsigned long index = desc->id % PCM_MAX_HANDLES;
    unsigned long version = desc->id - index;

    do {
        version += PCM_MAX_HANDLES;
    } while (version == 0);

    desc->id = version + index;
    free_desc_mask |= 1ul << index;
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

/* Globally lock out PCM playback interrupt */
void pcm_play_lock_device_callback(void)
{
    if (++pcm_play_locked == 1) {
        pcm_play_dma_lock();
    }
}

/* Globally reenable PCM playback interrupt */
void pcm_play_unlock_device_callback(void)
{
    if (--pcm_play_locked == 0) {
        pcm_play_dma_unlock();
    }
}

/* apply pcm settings to the hardware (internal use) */
static unsigned long pcm_apply_new_frequency(struct pcm_stream_desc *desc,
                                             int fsel)
{
    unsigned long sampr = hw_freq_sampr[fsel];

    hw_settings.samplerate = sampr;
    hw_settings.fsel = fsel;

    pcm_play_lock_device_callback();

    if (!pcm_mixer_apply_settings(desc, &hw_settings)) {
        if (pcm_play_dbl_buf_index >= 0) {
            pcm_playback_stop_dma();
        }
    }

    pcm_play_unlock_device_callback();

    logf(" pcm_dma_apply_settings");
    pcm_dma_apply_settings(&hw_settings);

    return sampr;
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
    unsigned int chnum = desc->format.chnum;

    for (unsigned int ch = chnum; ch--;) {
        peaks->peak[ch] = 0;
    }

    switch (desc->sample_size)
    {
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_CAPS)
    case 4:
    {
        const int32_t *p = peaks->addr;

        while (p < pend) {
            for (unsigned int ch = chnum; ch--;) {
                uint32_t s_abs = UABS(uint32_t, *p++);

                if (s_abs > peaks->peak[ch]) {
                    peaks->peak[ch] = s_abs;
                }
            }

            p += 3*chnum; /* every 4th sample only */
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

        while (p < pend) {
            for (unsigned int ch = chnum; ch--;) {
                uint32_t s_abs = UABS(uint32_t, *p++);

                if (s_abs > peaks->peak[ch]) {
                    peaks->peak[ch] = s_abs;
                }
            }

            p += 3*chnum; /* every 4th sample only */
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
        pcm_descs[i].id = 1ul*PCM_MAX_HANDLES + i;
    }

    free_desc_mask = MASK_N(uint32_t, PCM_MAX_HANDLES, 0);

    pcm_dma_init(&hw_settings);
    pcm_apply_new_frequency(NULL, HW_FREQ_DEFAULT);
    pcm_is_ready = true;
}

/* set frequency used by the audio hardware */
unsigned long pcm_set_frequency(pcm_handle_t pcm, unsigned long samplerate)
{
    logf("pcm_set_frequency:sr=%lu", samplerate);

    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return 0;
    }

#ifdef CONFIG_SAMPR_TYPES
    enum pcm_sampr_type type = IS_CAPTURE_STREAM(desc->flags) ?
                            SAMPR_TYPE_REC : SAMPR_TYPE_PLAY;

    /* For now, supported targets have direct conversion when configured with
     * CONFIG_SAMPR_TYPES.
     * Some hypothetical target with independent rates would need slightly
     * different handling throughout this source. */
    samplerate = pcm_sampr_convert(samplerate, type, SAMPR_TYPE_HW);
#endif /* CONFIG_SAMPR_TYPES */

    int index = round_value_to_list32(samplerate, hw_freq_sampr,
                                      HW_NUM_FREQ, false);

    if (samplerate != hw_freq_sampr[index])
        index = HW_FREQ_DEFAULT; /* Invalid = default */

    if (hw_settings.fsel != index) {
        samplerate = pcm_apply_new_frequency(desc, index);
    }

    /* return actual rate obtained */
#ifdef CONFIG_SAMPR_TYPES
    return pcm_sampr_convert(samplerate, SAMPR_TYPE_HW, type);
#else /* ndef CONFIG_SAMPR_TYPES */
    return samplerate;
#endif /* CONFIG_SAMPR_TYPES */
}

/* return last-set frequency */
unsigned long pcm_get_frequency(pcm_handle_t pcm)
{
    struct pcm_stream_desc *desc = pcm_handle_desc(pcm);
    if (!desc) {
        return 0;
    }

#ifdef CONFIG_SAMPR_TYPES
    enum pcm_sampr_type type = IS_CAPTURE_STREAM(desc->flags) ?
                            SAMPR_TYPE_REC : SAMPR_TYPE_PLAY;

    return pcm_sampr_convert(hw_settings.samplerate, SAMPR_TYPE_HW, type);
#else /* ndef CONFIG_SAMPR_TYPES */
    return hw_settings.samplerate;
#endif /* CONFIG_SAMPR_TYPES */
}


/** PCM Playback **/

/* Vet PCM format and setup dependent parts */
static NO_INLINE int setup_playback_format(struct pcm_stream_desc *desc,
                                 const struct pcm_format *format)
{
    copy_pcm_format(desc, format);
    format = &desc->format;

    int rc = pcm_mixer_format_config(desc);

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

/* Start the playback DMA transfer if not already going */
static void pcm_playback_start_dma(struct pcm_stream_desc *desc)
{
    pcm_mixer_activate_stream(desc);

    if (pcm_play_dbl_buf_index >= 0) {
        return;
    }

    /* PCM driver not running */
    memset(pcm_play_dbl_buf, 0x00, sizeof (pcm_play_dbl_buf));
    pcm_play_dbl_buf_index = 0;

    /* Two shortened silence frames to start cycle (1 frames latency total).
       First mixed frame will be done in parallel with 2nd buffer so that
       should be as long as possible */
    pcm_play_dbl_buf_frames[0] = 32; /* enough to top off any hw fifo */
    pcm_play_dbl_buf_frames[1] = PCM_DBL_BUF_FRAMES - 32;

#ifdef HAVE_SW_VOLUME_CONTROL
    /* Smoothed transition might not have happened so sync now */
    pcm_sw_volume_sync_factors();
#endif
    pcm_play_dma_prepare();
    pcm_play_dma_send_frames(pcm_play_dbl_buf[0], pcm_play_dbl_buf_frames[0]);
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
    if (desc->state != PCM_STATE_STOPPED) {
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

    peaks->addr = pcm_play_get_remaining_frames(desc, &peaks->frames);

    /* Keep reasonable limits on period */
    if (period < 1) {
        period = 1;
    }
    else if (period > HZ/5) {
        period = HZ/5;
    }

    peaks->period = (3*peaks->period + period) / 4;
    peaks->tick = tick;

    if (desc->state == PCM_STATE_RUNNING) {
        unsigned long lookahead = peaks->period*hw_settings.samplerate / HZ;
        peaks->frames = MIN(lookahead, peaks->frames);

        if (peaks->addr && peaks->frames) {
            peaks->end_addr = peaks->addr + peaks->frames*desc->frame_size;
            pcm_peak_peeker(desc, peaks);
        }
        /* else keep previous peak values */

        return;
    }

    pcm_zero_peaks(desc->format.chnum, peaks);
}

/* Called back by driver when last buffer has finished or an error occurs */
void pcm_play_dma_complete_callback(int status)
{
    bool cont = PCM_STATUS_CONTINUABLE(status);
    int index0 = pcm_play_dbl_buf_index; /* just played/next fill  */
    int index1 = 1 - index0;             /* next to send */

    pcm_play_dbl_buf_index = index1;

    if (cont) {
        unsigned long frames = pcm_play_dbl_buf_frames[index1];

        if (!frames) {
            /* no more */
            goto dma_finished;
        }

        pcm_play_dma_send_frames(pcm_play_dbl_buf[index1], frames);
    }
    /* else the fill callback is informed and playback will stop */

    pcm_play_dbl_buf_frames[index0] =
        pcm_mixer_fill_buffer(status, pcm_play_dbl_buf[index0],
                              PCM_DBL_BUF_FRAMES);

    if (!cont) {
    dma_finished:
        pcm_playback_stop_dma(); /* done or error */
    }
}

/* Play some audio */
int pcm_play_data(pcm_handle_t pcm,
                  pcm_play_callback_type callback,
                  const void *addr,
                  unsigned long frames,
                  const struct pcm_format *format)
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

        desc->addr        = addr;
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
static NO_INLINE int setup_record_format(struct pcm_stream_desc *desc,
                                         const struct pcm_format *format)
{
    copy_pcm_format(desc, format);
    format = &desc->format;

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
    pcm_rec_dma_prepare();
    pcm_rec_dma_capture_frames(desc->addr_rec, desc->frames);
}

/**
 * Return recording peaks - From the end of the last peak up to
 *                          current write position.
 */
static void pcm_recording_get_peaks(struct pcm_stream_desc *desc,
                                    struct pcm_peaks *peaks)
{
    if (desc->state != PCM_STATE_STOPPED) {
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

    pcm_zero_peaks(desc->format.chnum, peaks);
}

/* Called back by driver when last buffer is ready or an error occurs */
void pcm_rec_dma_complete_callback(int status)
{
    bool cont = PCM_STATUS_CONTINUABLE(status);

    /* recording DMA still runs when paused in order to keep level
       monitoring active - just use the same buffer again */
    struct pcm_stream_desc *desc = cur_rec_desc;

    if (desc->state == PCM_STATE_RUNNING) {
        desc->addr_rec = NULL;
        desc->frames = 0;

        pcm_record_callback_type callback = desc->callback_rec;

        if (callback) {
            status = callback(status, &desc->addr_rec, &desc->frames);
            ALIGN_AUDIOBUF(desc->addr_rec, desc->frames, 2*2);

            if (status != 0 || !(desc->addr_rec && desc->frames)) {
                cont = false;
            }
        }
    }

    if (cont) {
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
pcm_handle_t pcm_open(unsigned int flags)
{
    pcm_wait_for_init();

    struct pcm_stream_desc *desc = pcm_alloc_desc();
    if (!desc) {
        return 0;
    }

    desc->flags       = flags;
    desc->addr        = NULL;
    desc->frames      = 0;
    desc->amplitude   = PCM_AMP_UNITY;
    desc->prev_frames = 0;
    desc->callback    = NULL;
    desc->state       = PCM_STATE_STOPPED;

    int rc = 0;

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
        return 0;
    }
    else {
        return desc->id;
    }
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

    pcm_free_desc(desc);

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
const void * pcm_get_unplayed_data(pcm_handle_t pcm,
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
