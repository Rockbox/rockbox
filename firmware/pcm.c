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
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "general.h"

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
 *      pcm_play_dma_init
 *      pcm_play_dma_init
 *      pcm_play_dma_start
 *      pcm_play_dma_stop
 *      pcm_play_dma_pause
 *      pcm_play_dma_get_peak_buffer
 *   Data Read/Written within TSP -
 *      pcm_sampr (R)
 *      pcm_fsel (R)
 *      pcm_curr_sampr (R)
 *      pcm_callback_for_more (R)
 *      pcm_playing (R)
 *      pcm_paused (R)
 *
 * ==Playback/Recording==
 *   Semi-private -
 *      pcm_dma_apply_settings
 *
 * ==Recording==
 *   Public -
 *      pcm_rec_lock
 *      pcm_rec_unlock
 *   Semi-private -
 *      pcm_rec_dma_init
 *      pcm_rec_dma_close
 *      pcm_rec_dma_start
 *      pcm_rec_dma_stop
 *      pcm_rec_dma_get_peak_buffer
 *   Data Read/Written within TSP -
 *      pcm_rec_peak_addr (RW)
 *      pcm_callback_more_ready (R)
 *      pcm_recording (R)
 *
 * States are set _after_ the target's pcm driver is called so that it may
 * know from whence the state is changed. One exception is init.
 *
 */

/* the registered callback function to ask for more mp3 data */
volatile pcm_more_callback_type pcm_callback_for_more
    SHAREDBSS_ATTR = NULL;
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

/**
 * Do peak calculation using distance squared from axis and save a lot
 * of jumps and negation. Don't bother with the calculations of left or
 * right only as it's never really used and won't save much time.
 *
 * Used for recording and playback.
 */
static void pcm_peak_peeker(const void *addr, int count, int peaks[2])
{
    int32_t peak_l = 0, peak_r = 0;
    int32_t peaksq_l = 0, peaksq_r = 0;

    do
    {
        int32_t value = *(int32_t *)addr;
        int32_t ch, chsq;
#ifdef ROCKBOX_BIG_ENDIAN
        ch = value >> 16;
#else
        ch = (int16_t)value;
#endif
        chsq = ch*ch;
        if (chsq > peaksq_l)
            peak_l = ch, peaksq_l = chsq;

#ifdef ROCKBOX_BIG_ENDIAN
        ch = (int16_t)value;
#else
        ch = value >> 16;
#endif
        chsq = ch*ch;
        if (chsq > peaksq_r)
            peak_r = ch, peaksq_r = chsq;

        addr += 16;
        count -= 4;
    }
    while (count > 0);

    peaks[0] = abs(peak_l);
    peaks[1] = abs(peak_r);
}

void pcm_calculate_peaks(int *left, int *right)
{
    static int peaks[2] = { 0, 0 };
    static unsigned long last_peak_tick = 0;
    static unsigned long frame_period   = 0;

    long tick = current_tick;

    /* Throttled peak ahead based on calling period */
    long period = tick - last_peak_tick;

    /* Keep reasonable limits on period */
    if (period < 1)
        period = 1;
    else if (period > HZ/5)
        period = HZ/5;

    frame_period = (3*frame_period + period) >> 2;

    last_peak_tick = tick;

    if (pcm_playing && !pcm_paused)
    {
        const void *addr;
        int count, framecount;

        addr = pcm_play_dma_get_peak_buffer(&count);

        framecount = frame_period*pcm_curr_sampr / HZ;
        count = MIN(framecount, count);

        if (count > 0)
            pcm_peak_peeker(addr, count, peaks);
    }
    else
    {
        peaks[0] = peaks[1] = 0;
    }

    if (left)
        *left = peaks[0];

    if (right)
        *right = peaks[1];
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

    pcm_play_dma_stopped_callback();

    pcm_set_frequency(HW_SAMPR_DEFAULT);

    logf(" pcm_play_dma_init");
    pcm_play_dma_init();
}

/* Common code to pcm_play_data and pcm_play_pause */
static void pcm_play_data_start(unsigned char *start, size_t size)
{
    if (!(start && size))
    {
        pcm_more_callback_type get_more = pcm_callback_for_more;
        size = 0;
        if (get_more)
        {
            logf(" get_more");
            get_more(&start, &size);
        }
    }

    if (start && size)
    {
        logf(" pcm_play_dma_start");
        pcm_apply_settings();
        pcm_play_dma_start(start, size);
        pcm_playing = true;
        pcm_paused = false;
        return;
    }

    /* Force a stop */
    logf(" pcm_play_dma_stop");
    pcm_play_dma_stop();
    pcm_play_dma_stopped_callback();
}

void pcm_play_data(pcm_more_callback_type get_more,
                   unsigned char *start, size_t size)
{
    logf("pcm_play_data");

    pcm_play_lock();

    pcm_callback_for_more = get_more;

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
        pcm_play_dma_stopped_callback();
    }
    else
    {
        logf(" not playing");
    }

    pcm_play_unlock();
}

void pcm_play_dma_stopped_callback(void)
{
    pcm_callback_for_more = NULL;
    pcm_paused = false;
    pcm_playing = false;
}

/**/

/* set frequency next frequency used by the audio hardware -
 * what pcm_apply_settings will set */
void pcm_set_frequency(unsigned int samplerate)
{
    logf("pcm_set_frequency");

    int index = round_value_to_list32(samplerate, hw_freq_sampr,
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

    if (pcm_sampr != pcm_curr_sampr)
    {
        logf(" pcm_dma_apply_settings");
        pcm_dma_apply_settings();
        pcm_curr_sampr = pcm_sampr;
    }
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

void pcm_mute(bool mute)
{
#ifndef SIMULATOR
    audiohw_mute(mute);
#endif

    if (mute)
        sleep(HZ/16);
}

#ifdef HAVE_RECORDING
/** Low level pcm recording apis **/

/* Next start for recording peaks */
const volatile void *pcm_rec_peak_addr SHAREDBSS_ATTR = NULL;
/* the registered callback function for when more data is available */
volatile pcm_more_callback_type2
    pcm_callback_more_ready SHAREDBSS_ATTR = NULL;
/* DMA transfer in is currently active */
volatile bool pcm_recording SHAREDBSS_ATTR = false;

/**
 * Return recording peaks - From the end of the last peak up to
 *                          current write position.
 */
void pcm_calculate_rec_peaks(int *left, int *right)
{
    static int peaks[2];

    if (pcm_recording)
    {
        const void *addr;
        int count;

        addr = pcm_rec_dma_get_peak_buffer(&count);

        if (count > 0)
        {
            pcm_peak_peeker(addr, count, peaks);

            if (addr == pcm_rec_peak_addr)
                pcm_rec_peak_addr = (int32_t *)addr + count;
        }
    }
    else
    {
        peaks[0] = peaks[1] = 0;
    }

    if (left)
        *left = peaks[0];

    if (right)
        *right = peaks[1];
} /* pcm_calculate_rec_peaks */

/****************************************************************************
 * Functions that do not require targeted implementation but only a targeted
 * interface
 */
void pcm_init_recording(void)
{
    logf("pcm_init_recording");

    /* Recording init is locked unlike general pcm init since this is not
     * just a one-time event at startup and it should and must be safe by
     * now. */
    pcm_rec_lock();

    logf(" pcm_rec_dma_init");
    pcm_rec_dma_stopped_callback();
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
        pcm_rec_dma_stopped_callback();
    }

    logf(" pcm_rec_dma_close");
    pcm_rec_dma_close();

    pcm_rec_unlock();
}

void pcm_record_data(pcm_more_callback_type2 more_ready,
                     void *start, size_t size)
{
    logf("pcm_record_data");

    if (!(start && size))
    {
        logf(" no buffer");
        return;
    }

    pcm_rec_lock();

    pcm_callback_more_ready = more_ready;

    logf(" pcm_rec_dma_start");
    pcm_apply_settings();
    pcm_rec_dma_start(start, size);
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
        pcm_rec_dma_stopped_callback();
    }

    pcm_rec_unlock();
} /* pcm_stop_recording */

bool pcm_is_recording(void)
{
    return pcm_recording;
}

void pcm_rec_dma_stopped_callback(void)
{
    pcm_recording = false;
    pcm_callback_more_ready = NULL;
}

#endif /* HAVE_RECORDING */
