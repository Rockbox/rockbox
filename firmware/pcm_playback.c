/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"

/**
 * APIs implemented in the target-specific portion:
 * Public -
 *      pcm_init
 *      pcm_get_bytes_waiting
 *      pcm_calculate_peaks
 * Semi-private -
 *      pcm_play_dma_start
 *      pcm_play_dma_stop
 *      pcm_play_pause_pause
 *      pcm_play_pause_unpause
 */

/** These items may be implemented target specifically or need to
    be shared semi-privately **/

/* the registered callback function to ask for more mp3 data */
volatile pcm_more_callback_type pcm_callback_for_more = NULL;
volatile bool                   pcm_playing           = false;
volatile bool                   pcm_paused            = false;

void pcm_play_dma_start(const void *addr, size_t size);
void pcm_play_dma_stop(void);
void pcm_play_pause_pause(void);
void pcm_play_pause_unpause(void);

/** Functions that require targeted implementation **/

#if defined(CPU_COLDFIRE) || (CONFIG_CPU == S3C2440) || (CONFIG_CPU == IMX31L)
/* Implemented in target/... */
#else
/* dummy functions for those not actually supporting all this yet */
void pcm_apply_settings(void)
{
}
/** **/

void pcm_mute(bool mute)
{
#if defined(HAVE_WM8975) || defined(HAVE_WM8758) \
   || defined(HAVE_WM8731) || defined(HAVE_WM8721)
    audiohw_mute(mute);
#endif
    if (mute)
        sleep(HZ/16);
}
#endif /* defined(CPU_COLDFIRE) || (CONFIG_CPU == S3C2440) */

#if defined(CPU_COLDFIRE) || (CONFIG_CPU == S3C2440) || defined(CPU_PP) \
    || (CONFIG_CPU == IMX31L)
/* Implemented in target/... */
#else
static int pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */

unsigned short* p IBSS_ATTR;
size_t p_size IBSS_ATTR;

void pcm_play_dma_start(const void *addr, size_t size)
{
    p = (unsigned short*)addr;
    p_size = size;

    pcm_playing = true;
}

void pcm_play_dma_stop(void)
{
    pcm_playing = false;
    if (!audio_status())
        pcm_paused = false;
}

void pcm_play_pause_pause(void)
{
}

void pcm_play_pause_unpause(void)
{
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
    pcm_freq = HW_SAMPR_DEFAULT;
}
size_t pcm_get_bytes_waiting(void)
{
    return p_size;
}

/*
 * This function goes directly into the DMA buffer to calculate the left and
 * right peak values. To avoid missing peaks it tries to look forward two full
 * peek periods (2/HZ sec, 100% overlap), although it's always possible that
 * the entire period will not be visible. To reduce CPU load it only looks at
 * every third sample, and this can be reduced even further if needed (even
 * every tenth sample would still be pretty accurate).
 */

/* Check for a peak every PEAK_STRIDE samples */
#define PEAK_STRIDE   3
/* Up to 1/50th of a second of audio for peak calculation */
/* This should use NATIVE_FREQUENCY, or eventually an adjustable freq. value */
#define PEAK_SAMPLES  (44100/50)
void pcm_calculate_peaks(int *left, int *right)
{
    short *addr;
    short *end;
    {
        size_t samples = p_size / 4;
        addr = p;

        if (samples > PEAK_SAMPLES)
            samples = PEAK_SAMPLES - (PEAK_STRIDE - 1);
        else
            samples -= MIN(PEAK_STRIDE - 1, samples);

        end = &addr[samples * 2];
    }

    if (left && right) {
        int left_peak = 0, right_peak = 0;

        while (addr < end) {
            int value;
            if ((value = addr [0]) > left_peak)
                left_peak = value;
            else if (-value > left_peak)
                left_peak = -value;

            if ((value = addr [PEAK_STRIDE | 1]) > right_peak)
                right_peak = value;
            else if (-value > right_peak)
                right_peak = -value;

            addr = &addr[PEAK_STRIDE * 2];
        }

        *left = left_peak;
        *right = right_peak;
    }
    else if (left || right) {
        int peak_value = 0, value;

        if (right)
            addr += (PEAK_STRIDE | 1);

        while (addr < end) {
            if ((value = addr [0]) > peak_value)
                peak_value = value;
            else if (-value > peak_value)
                peak_value = -value;

            addr += PEAK_STRIDE * 2;
        }

        if (left)
            *left = peak_value;
        else
            *right = peak_value;
    }
}
#endif /* defined(CPU_COLDFIRE) || (CONFIG_CPU == S3C2440) || defined(CPU_PP) */

/****************************************************************************
 * Functions that do not require targeted implementation but only a targeted
 * interface
 */

/* Common code to pcm_play_data and pcm_play_pause
   Returns true if DMA playback was started, else false. */
bool pcm_play_data_start(pcm_more_callback_type get_more,
                         unsigned char *start, size_t size)
{
    if (!(start && size))
    {
        size = 0;
        if (get_more)
            get_more(&start, &size);
    }

    if (start && size)
    {
        pcm_play_dma_start(start, size);
        return true;
    }

    return false;
}

void pcm_play_data(pcm_more_callback_type get_more,
                   unsigned char *start, size_t size)
{
    pcm_callback_for_more = get_more;

    if (pcm_play_data_start(get_more, start, size) && pcm_paused)
    {
        pcm_paused = false;
        pcm_play_pause(false);
    }
}

void pcm_play_pause(bool play)
{
    bool needs_change = pcm_paused == play;

    /* This needs to be done ahead of the rest to prevent infinite
       recursion from pcm_play_data */
    pcm_paused = !play;

    if (pcm_playing && needs_change)
    {
        if (play)
        {
            if (pcm_get_bytes_waiting())
            {
                logf("unpause");
                pcm_play_pause_unpause();
            }
            else
            {
                logf("unpause, no data waiting");
                if (!pcm_play_data_start(pcm_callback_for_more, NULL, 0))
                {
                    pcm_play_dma_stop();
                    logf("unpause attempted, no data");
                }
            }
        }
        else
        {
            logf("pause");
            pcm_play_pause_pause();
        }
    } /* pcm_playing && needs_change */
}

void pcm_play_stop(void)
{
    if (pcm_playing)
        pcm_play_dma_stop();
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

