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
#include "pcm-internal.h"
#include "pcm_mixer.h"
#ifdef HAVE_SW_VOLUME_CONTROL
#include "dsp-util.h"
#endif /* HAVE_SW_VOLUME_CONTROL */

/**
 * Aspects implemented in the target-specific portion:
 *
 * ==Playback==
 *   Public -
 *      pcm_postinit
 *      pcm_get_bytes_waiting
 *      pcm_play_lock
 *      pcm_play_unlock
 *   Semi-private -
 *      pcm_play_dma_complete_callback
 *      pcm_play_dma_status_callback
 *      pcm_play_dma_init
 *      pcm_play_dma_postinit
 *      pcm_play_dma_start
 *      pcm_play_dma_stop
 *      pcm_play_dma_pause
 *      pcm_play_dma_get_peak_buffer
 *   Data Read/Written within TSP -
 *      pcm_sampr (R)
 *      pcm_fsel (R)
 *      pcm_curr_sampr (R)
 *      pcm_playing (R)
 *      pcm_paused (R)
 *
 * ==Playback/Recording==
 *   Public -
 *      pcm_dma_addr
 *   Semi-private -
 *      pcm_dma_apply_settings
 *
 * ==Recording==
 *   Public -
 *      pcm_rec_lock
 *      pcm_rec_unlock
 *   Semi-private -
 *      pcm_rec_dma_complete_callback
 *      pcm_rec_dma_status_callback
 *      pcm_rec_dma_init
 *      pcm_rec_dma_close
 *      pcm_rec_dma_start
 *      pcm_rec_dma_stop
 *      pcm_rec_dma_get_peak_buffer
 *   Data Read/Written within TSP -
 *      pcm_recording (R)
 *
 * States are set _after_ the target's pcm driver is called so that it may
 * know from whence the state is changed. One exception is init.
 *
 */

/* 'true' when all stages of pcm initialization have completed */
static bool pcm_is_ready = false;

/* The registered callback function to ask for more mp3 data */
static volatile pcm_play_callback_type
    pcm_callback_for_more SHAREDBSS_ATTR = NULL;
/* The registered callback function to inform of DMA status */
volatile pcm_status_callback_type
    pcm_play_status_callback SHAREDBSS_ATTR = NULL;
/* PCM playback state */
volatile bool pcm_playing SHAREDBSS_ATTR = false;
/* PCM paused state. paused implies playing */
volatile bool pcm_paused SHAREDBSS_ATTR = false;
/* samplerate of currently playing audio - undefined if stopped */
unsigned long pcm_curr_sampr SHAREDBSS_ATTR = 0;
/* samplerate waiting to be set */
unsigned long pcm_sampr SHAREDBSS_ATTR = HW_SAMPR_DEFAULT;
/* samplerate frequency selection index */
int pcm_fsel SHAREDBSS_ATTR = HW_FREQ_DEFAULT;


/** SW Volume Control data */
#ifdef HAVE_SW_VOLUME_CONTROL

/* FIXME: fixedpoint.c is in /apps for now and this is /firmware */
#define fp_mul(x, y, z) (long)((((long long)(x)) * ((long long)(y))) >> (z))
extern long fp_factor(long decibels, unsigned int fracbits);

/* source buffer from client */
static const void * volatile src_buf_addr = NULL;
static size_t volatile src_buf_rem = 0;

#define PCM_FACTOR_UNITY (1 << PCM_FACTOR_BITS)
#define PCM_SAMPLE_SIZE (2 * sizeof (int16_t))
#define PCM_PLAY_DBL_BUF_SIZE (PCM_PLAY_DBL_BUF_SAMPLE*PCM_SAMPLE_SIZE)

/* double buffer and frame length control */
static int16_t pcm_dbl_buf[2][PCM_PLAY_DBL_BUF_SAMPLES*2]
        PCM_DBL_BUF_BSS MEM_ALIGN_ATTR;
static size_t pcm_dbl_buf_size[2];
static int pcm_dbl_buf_num = 0;
static size_t frame_size;
static unsigned int frame_count, frame_err, frame_frac;

/* pcm scaling coefficients */
static int32_t pcm_prescale_factor = PCM_FACTOR_UNITY;
static int32_t pcm_vol_factor_l = 0, pcm_vol_factor_r = 0;
static int32_t pcm_factor_l = 0, pcm_factor_r = 0;

#endif /* HAVE_SW_VOLUME_CONTROL */

/* Call registered callback to obtain next buffer */
static inline bool get_more_int(const void **addr, size_t *size)
{
    pcm_play_callback_type get_more = pcm_callback_for_more;

    if (UNLIKELY(!get_more))
        return false;

    *addr = NULL;
    *size = 0;
    get_more(addr, size);
    ALIGN_AUDIOBUF(*addr, *size);

    return *addr && *size;
}

/* Called internally by functions to stop hardware and reset the state */
static void pcm_play_dma_stop_int(void)
{
    pcm_play_dma_stop();

#ifdef HAVE_SW_VOLUME_CONTROL
    src_buf_addr = NULL;
    src_buf_rem = 0;
#endif /* HAVE_SW_VOLUME_CONTROL */

    pcm_callback_for_more = NULL;
    pcm_play_status_callback = NULL;
    pcm_paused = false;
    pcm_playing = false;
}

static void pcm_wait_for_init(void)
{
    while (!pcm_is_ready)
        sleep(0);
}

#ifdef HAVE_SW_VOLUME_CONTROL
/** SW Volume Control functions **/

/* TODO: #include CPU-optimized routines and move this to /firmware/asm */
static inline void pcm_copy_buffer(int16_t *dst, const int16_t *src,
                                   size_t size)
{
    int32_t factor_l = pcm_factor_l;
    int32_t factor_r = pcm_factor_r;

    if (LIKELY(factor_l <= PCM_FACTOR_UNITY && factor_r <= PCM_FACTOR_UNITY))
    {
        /* All cut or unity */
        while (size)
        {
            *dst++ = (factor_l * *src++ +
                      PCM_FACTOR_UNITY/2) >> PCM_FACTOR_BITS;
            *dst++ = (factor_r * *src++ +
                      PCM_FACTOR_UNITY/2) >> PCM_FACTOR_BITS;
            size -= PCM_SAMPLE_SIZE;
        }
    }
    else
    {
        /* Any positive gain requires clipping */
        while (size)
        {
            *dst++ = clip_sample_16((factor_l * *src++ +
                                    PCM_FACTOR_UNITY/2) >> PCM_FACTOR_BITS);
            *dst++ = clip_sample_16((factor_r * *src++ +
                                    PCM_FACTOR_UNITY/2) >> PCM_FACTOR_BITS);
            size -= PCM_SAMPLE_SIZE;
        }
    }
}

bool pcm_play_dma_complete_callback(enum pcm_dma_status status,
                                    const void **addr, size_t *size)
{
    if (status < PCM_DMAST_OK)
        status = pcm_play_call_status_cb(status);

    const void *p = pcm_dbl_buf[pcm_dbl_buf_num];
    size_t sz = pcm_dbl_buf_size[pcm_dbl_buf_num];

    if (status >= PCM_DMAST_OK && p && sz)
    {
        *addr = p;
        *size = sz;
        return true;
    }
    else
    {
        pcm_play_dma_stop_int();
        return false;
    }
}

/* Equitably divide large source buffers amongst double buffer frames;
   frames smaller than or equal to the double buffer chunk size will play
   in one chunk */
static void update_frame_params(size_t size)
{
    int count    = size / PCM_SAMPLE_SIZE;
    frame_count  = (count + PCM_PLAY_DBL_BUF_SAMPLES - 1) /
                   PCM_PLAY_DBL_BUF_SAMPLES;
    int perframe = count / frame_count;
    frame_size   = perframe * PCM_SAMPLE_SIZE;
    frame_frac   = count - perframe * frame_count;
    frame_err    = 0;
}

/* Obtain the next buffer and prepare it for pcm driver playback */
enum pcm_dma_status
pcm_play_dma_status_callback_int(enum pcm_dma_status status)
{
    if (status != PCM_DMAST_STARTED)
        return status;

    size_t size = pcm_dbl_buf_size[pcm_dbl_buf_num];
    const void *addr = src_buf_addr + size;

    size = src_buf_rem - size;

    if (size == 0 && get_more_int(&addr, &size))
    {
        update_frame_params(size);
        pcm_play_call_status_cb(PCM_DMAST_STARTED);
    }

    src_buf_addr = addr;
    src_buf_rem = size;

    if (size != 0)
    {
        size = frame_size;

        if ((frame_err += frame_frac) >= frame_count)
        {
            frame_err -= frame_count;
            size += PCM_SAMPLE_SIZE;
        }
    }

    pcm_dbl_buf_num ^= 1;
    pcm_dbl_buf_size[pcm_dbl_buf_num] = size;
    pcm_copy_buffer(pcm_dbl_buf[pcm_dbl_buf_num], addr, size);

    return PCM_DMAST_OK;
}

/* Prefill double buffer and start pcm driver */
static void pcm_play_dma_start_int(const void *addr, size_t size)
{
    src_buf_addr = addr;
    src_buf_rem = size;
    pcm_dbl_buf_num = 0;
    pcm_dbl_buf_size[0] = 0;

    update_frame_params(size);
    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    pcm_play_dma_status_callback(PCM_DMAST_STARTED);

    pcm_play_dma_start(pcm_dbl_buf[1], pcm_dbl_buf_size[1]);
}

/* Return playing buffer from the source buffer */
static const void * pcm_play_dma_get_peak_buffer_int(int *count)
{
    const void *addr = src_buf_addr;
    size_t size = src_buf_rem;
    const void *addr2 = src_buf_addr;

    if (addr == addr2)
    {
        *count = size / PCM_SAMPLE_SIZE;
        return addr;
    }

    *count = 0;
    return NULL;
}

/* Return the scale factor corresponding to the centibel level */
static int32_t pcm_centibels_to_factor(int volume)
{
    if (volume == INT_MIN)
        return 0; /* mute */

    /* Centibels -> fixedpoint */
    return fp_factor(PCM_FACTOR_UNITY*volume / 10, PCM_FACTOR_BITS);
}

/* Produce final pcm scale factor */
static void pcm_sync_prescaler(void)
{
    int32_t factor_l = fp_mul(pcm_prescale_factor, pcm_vol_factor_l,
                              PCM_FACTOR_BITS);

    if (factor_l < PCM_FACTOR_MIN)
        factor_l = PCM_FACTOR_MIN;
    else if (factor_l > PCM_FACTOR_MAX)
        factor_l = PCM_FACTOR_MAX;

    int32_t factor_r = fp_mul(pcm_prescale_factor, pcm_vol_factor_r,
                              PCM_FACTOR_BITS);

    if (factor_r < PCM_FACTOR_MIN)
        factor_r = PCM_FACTOR_MIN;
    else if (factor_r > PCM_FACTOR_MAX)
        factor_r = PCM_FACTOR_MAX;

    pcm_factor_l = factor_l;
    pcm_factor_r = factor_r;
}

/* Set the prescaler value for all PCM playback */
void pcm_set_prescaler(int prescale)
{
    pcm_prescale_factor = pcm_centibels_to_factor(-prescale);
    pcm_sync_prescaler();
}

/* Set the per-channel volume cut/gain for all PCM playback */
void pcm_set_master_volume(int vol_l, int vol_r)
{
    pcm_vol_factor_l = pcm_centibels_to_factor(vol_l);
    pcm_vol_factor_r = pcm_centibels_to_factor(vol_r);
    pcm_sync_prescaler();
}

#else /* ndef HAVE_SW_VOLUME_CONTROL */
/** Standard functions **/

static inline void pcm_play_dma_start_int(const void *addr, size_t size)
{
    pcm_play_dma_start(addr, size);
}

static inline const void * pcm_play_dma_get_peak_buffer_int(int *count)
{
    return pcm_play_dma_get_peak_buffer(count);
}

bool pcm_play_dma_complete_callback(enum pcm_dma_status status,
                                    const void **addr, size_t *size)
{
    /* Check status callback first if error */
    if (status < PCM_DMAST_OK)
        status = pcm_play_dma_status_callback(status);

    if (status >= PCM_DMAST_OK && get_more_int(addr, size))
        return true;

    /* Error, callback missing or no more DMA to do */
    pcm_play_dma_stop_int();
    return false;
}
#endif /* HAVE_SW_VOLUME_CONTROL */

/* Common code to pcm_play_data and pcm_play_pause */
static void pcm_play_data_start(const void *addr, size_t size)
{
    ALIGN_AUDIOBUF(addr, size);

    if ((addr && size) || get_more_int(&addr, &size))
    {
        logf(" pcm_play_dma_start");
        pcm_apply_settings();
        pcm_play_dma_start_int(addr, size);
        pcm_playing = true;
        pcm_paused = false;
    }
    else
    {
        /* Force a stop */
        logf(" pcm_play_dma_stop");
        pcm_play_dma_stop_int();
    }
}

/**
 * Perform peak calculation on a buffer of packed 16-bit samples.
 *
 * Used for recording and playback.
 */
static void pcm_peak_peeker(const int16_t *p, int count,
                            struct pcm_peaks *peaks)
{
    uint32_t peak_l = 0, peak_r = 0;
    const int16_t *pend = p + 2 * count;

    do
    {
        int32_t s;

        s = p[0];

        if (s < 0)
            s = -s;

        if ((uint32_t)s > peak_l)
            peak_l = s;

        s = p[1];

        if (s < 0)
            s = -s;

        if ((uint32_t)s > peak_r)
            peak_r = s;

        p += 4 * 2; /* Every 4th sample, interleaved */
    }
    while (p < pend);

    peaks->left = peak_l;
    peaks->right = peak_r;
}

void pcm_do_peak_calculation(struct pcm_peaks *peaks, bool active,
                             const void *addr, int count)
{
    long tick = current_tick;

    /* Peak no farther ahead than expected period to avoid overcalculation */
    long period = tick - peaks->tick;

    /* Keep reasonable limits on period */
    if (period < 1)
        period = 1;
    else if (period > HZ/5)
        period = HZ/5;

    peaks->period = (3*peaks->period + period) / 4;
    peaks->tick = tick;

    if (active)
    {
        int framecount = peaks->period*pcm_curr_sampr / HZ;
        count = MIN(framecount, count);

        if (count > 0)
            pcm_peak_peeker(addr, count, peaks);
        /* else keep previous peak values */
    }
    else
    {
        /* peaks are zero */
        peaks->left = peaks->right = 0;
    }
}

void pcm_calculate_peaks(int *left, int *right)
{
    /* peak data for the global peak values - i.e. what the final output is */
    static struct pcm_peaks peaks;

    int count;
    const void *addr = pcm_play_dma_get_peak_buffer_int(&count);

    pcm_do_peak_calculation(&peaks, pcm_playing && !pcm_paused,
                            addr, count);

    if (left)
        *left = peaks.left;

    if (right)
        *right = peaks.right;
}

const void * pcm_get_peak_buffer(int *count)
{
    return pcm_play_dma_get_peak_buffer_int(count);
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

/****************************************************************************
 * Functions that do not require targeted implementation but only a targeted
 * interface
 */

/* This should only be called at startup before any audio playback or
   recording is attempted */
void pcm_init(void)
{
    logf("pcm_init");

    pcm_set_frequency(HW_SAMPR_DEFAULT);

    logf(" pcm_play_dma_init");
    pcm_play_dma_init();
}

/* Finish delayed init */
void pcm_postinit(void)
{
    logf("pcm_postinit");

    logf(" pcm_play_dma_postinit");

    pcm_play_dma_postinit();

    pcm_is_ready = true;
}

bool pcm_is_initialized(void)
{
    return pcm_is_ready;
}

void pcm_play_data(pcm_play_callback_type get_more,
                   pcm_status_callback_type status_cb,
                   const void *start, size_t size)
{
    logf("pcm_play_data");

    pcm_play_lock();

    pcm_callback_for_more = get_more;
    pcm_play_status_callback = status_cb;

    logf(" pcm_play_data_start");
    pcm_play_data_start(start, size);

    pcm_play_unlock();
}

void pcm_play_pause(bool play)
{
    logf("pcm_play_pause: %s", play ? "play" : "pause");

    pcm_play_lock();

    if (play == pcm_paused && pcm_playing)
    {
        if (!play)
        {
            logf(" pcm_play_dma_pause");
            pcm_play_dma_pause(true);
            pcm_paused = true;
        }
        else if (pcm_get_bytes_waiting() > 0)
        {
            logf(" pcm_play_dma_pause");
            pcm_apply_settings();
            pcm_play_dma_pause(false);
            pcm_paused = false;
        }
        else
        {
            logf(" pcm_play_dma_start: no data");
#ifdef HAVE_SW_VOLUME_CONTROL
            pcm_play_data_start(src_buf_addr, src_buf_rem);
#else
            pcm_play_data_start(NULL, 0);
#endif
        }
    }
    else
    {
        logf(" no change");
    }

    pcm_play_unlock();
}

void pcm_play_stop(void)
{
    logf("pcm_play_stop");

    pcm_play_lock();

    if (pcm_playing)
    {
        logf(" pcm_play_dma_stop");
        pcm_play_dma_stop_int();
    }
    else
    {
        logf(" not playing");
    }

    pcm_play_unlock();
}

/**/

/* set frequency next frequency used by the audio hardware -
 * what pcm_apply_settings will set */
void pcm_set_frequency(unsigned int samplerate)
{
    logf("pcm_set_frequency");

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

    pcm_sampr = hw_freq_sampr[index];
    pcm_fsel = index;
}

/* apply pcm settings to the hardware */
void pcm_apply_settings(void)
{
    logf("pcm_apply_settings");

    pcm_wait_for_init();

    if (pcm_sampr != pcm_curr_sampr)
    {
        logf(" pcm_dma_apply_settings");
        pcm_dma_apply_settings();
        pcm_curr_sampr = pcm_sampr;
    }
}

#ifdef HAVE_RECORDING
/** Low level pcm recording apis **/

/* Next start for recording peaks */
static const void * volatile pcm_rec_peak_addr SHAREDBSS_ATTR = NULL;
/* the registered callback function for when more data is available */
static volatile pcm_rec_callback_type
    pcm_callback_more_ready SHAREDBSS_ATTR = NULL;
volatile pcm_status_callback_type
    pcm_rec_status_callback SHAREDBSS_ATTR = NULL;
/* DMA transfer in is currently active */
volatile bool pcm_recording SHAREDBSS_ATTR = false;

/* Called internally by functions to reset the state */
static void pcm_recording_stopped(void)
{
    pcm_recording = false;
    pcm_callback_more_ready = NULL;
    pcm_rec_status_callback = NULL;
}

/**
 * Return recording peaks - From the end of the last peak up to
 *                          current write position.
 */
void pcm_calculate_rec_peaks(int *left, int *right)
{
    static struct pcm_peaks peaks;

    if (pcm_recording)
    {
        const int16_t *peak_addr = pcm_rec_peak_addr;
        const int16_t *addr = pcm_rec_dma_get_peak_buffer();

        if (addr != NULL)
        {
            int count = (addr - peak_addr) / 2; /* Interleaved L+R */

            if (count > 0)
            {
                pcm_peak_peeker(peak_addr, count, &peaks);

                if (peak_addr == pcm_rec_peak_addr)
                    pcm_rec_peak_addr = addr;
            }
        }
        /* else keep previous peak values */
    }
    else
    {
        peaks.left = peaks.right = 0;
    }

    if (left)
        *left = peaks.left;

    if (right)
        *right = peaks.right;
}

bool pcm_is_recording(void)
{
    return pcm_recording;
}

/****************************************************************************
 * Functions that do not require targeted implementation but only a targeted
 * interface
 */

void pcm_init_recording(void)
{
    logf("pcm_init_recording");

    pcm_wait_for_init();

    /* Stop the beasty before attempting recording */
    mixer_reset();

    /* Recording init is locked unlike general pcm init since this is not
     * just a one-time event at startup and it should and must be safe by
     * now. */
    pcm_rec_lock();

    logf(" pcm_rec_dma_init");
    pcm_recording_stopped();
    pcm_rec_dma_init();

    pcm_rec_unlock();
}

void pcm_close_recording(void)
{
    logf("pcm_close_recording");

    pcm_rec_lock();

    if (pcm_recording)
    {
        logf(" pcm_rec_dma_stop");
        pcm_rec_dma_stop();
        pcm_recording_stopped();
    }

    logf(" pcm_rec_dma_close");
    pcm_rec_dma_close();

    pcm_rec_unlock();
}

void pcm_record_data(pcm_rec_callback_type more_ready,
                     pcm_status_callback_type status_cb,
                     void *addr, size_t size)
{
    logf("pcm_record_data");

    ALIGN_AUDIOBUF(addr, size);

    if (!(addr && size))
    {
        logf(" no buffer");
        return;
    }

    pcm_rec_lock();

    pcm_callback_more_ready = more_ready;
    pcm_rec_status_callback = status_cb;

    /* Need a physical DMA address translation, if not already physical. */
    pcm_rec_peak_addr = pcm_rec_dma_addr(addr);

    logf(" pcm_rec_dma_start");
    pcm_apply_settings();
    pcm_rec_dma_start(addr, size);
    pcm_recording = true;

    pcm_rec_unlock();
} /* pcm_record_data */

void pcm_stop_recording(void)
{
    logf("pcm_stop_recording");

    pcm_rec_lock();

    if (pcm_recording)
    {
        logf(" pcm_rec_dma_stop");
        pcm_rec_dma_stop();
        pcm_recording_stopped();
    }

    pcm_rec_unlock();
} /* pcm_stop_recording */

bool pcm_rec_dma_complete_callback(enum pcm_dma_status status,
                                   void **addr, size_t *size)
{
     /* Check status callback first if error */
    if (status < PCM_DMAST_OK)
        status = pcm_rec_dma_status_callback(status);

    pcm_rec_callback_type have_more = pcm_callback_more_ready;

    if (have_more && status >= PCM_DMAST_OK)
    {
        /* Call registered callback to obtain next buffer */
        have_more(addr, size);
        ALIGN_AUDIOBUF(*addr, *size);

        if (*addr && *size)
        {
            /* Need a physical DMA address translation, if not already
             * physical. */
            pcm_rec_peak_addr = pcm_rec_dma_addr(*addr);
            return true;
        }
    }

    /* Error, callback missing or no more DMA to do */
    pcm_rec_dma_stop();
    pcm_recording_stopped();

    return false;
}

#endif /* HAVE_RECORDING */
