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

/* Called internally by functions to reset the state */
static void pcm_play_stopped(void)
{
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
    const void *addr = pcm_play_dma_get_peak_buffer(&count);

    pcm_do_peak_calculation(&peaks, pcm_playing && !pcm_paused,
                            addr, count);

    if (left)
        *left = peaks.left;

    if (right)
        *right = peaks.right;
}

const void* pcm_get_peak_buffer(int * count)
{
    return pcm_play_dma_get_peak_buffer(count);
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

    pcm_play_stopped();

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

/* Common code to pcm_play_data and pcm_play_pause */
static void pcm_play_data_start(const void *addr, size_t size)
{
    ALIGN_AUDIOBUF(addr, size);

    if (!(addr && size))
    {
        pcm_play_callback_type get_more = pcm_callback_for_more;
        addr = NULL;
        size = 0;

        if (get_more)
        {
            logf(" get_more");
            get_more(&addr, &size);
            ALIGN_AUDIOBUF(addr, size);
        }
    }

    if (addr && size)
    {
        logf(" pcm_play_dma_start");
        pcm_apply_settings();
        pcm_play_dma_start(addr, size);
        pcm_playing = true;
        pcm_paused = false;
        return;
    }

    /* Force a stop */
    logf(" pcm_play_dma_stop");
    pcm_play_dma_stop();
    pcm_play_stopped();
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

bool pcm_play_dma_complete_callback(enum pcm_dma_status status,
                                    const void **addr, size_t *size)
{
    /* Check status callback first if error */
    if (status < PCM_DMAST_OK)
        status = pcm_play_dma_status_callback(status);

    pcm_play_callback_type get_more = pcm_callback_for_more;

    if (get_more && status >= PCM_DMAST_OK)
    {
        *addr = NULL;
        *size = 0;

        /* Call registered callback to obtain next buffer */
        get_more(addr, size);
        ALIGN_AUDIOBUF(*addr, *size);

        if (*addr && *size)
            return true;
    } 

    /* Error, callback missing or no more DMA to do */
    pcm_play_dma_stop();
    pcm_play_stopped();

    return false;
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
            pcm_play_data_start(NULL, 0);
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
        pcm_play_dma_stop();
        pcm_play_stopped();
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
