/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 Michael Sevakis
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
#include "settings.h"
#include "pcm.h"
#include "misc.h"
#include "fixedpoint.h"

/** Beep generation, CPU optimized **/
#include "asm/beep.c"

static pcm_handle_t pcm_handle;  /* Opened PCM stream (always open) */
static uint32_t beep_phase;      /* Phase of square wave generator */
static uint32_t beep_step;       /* Step of square wave generator on each sample */
static unsigned long beep_rem;   /* Number of frames remaining to generate */

#define BEEP_FRAMES(fs, duration) ((fs) / 1000 * (duration))

/* Reserve enough static space for keyclick to fit in worst case */
#define BEEP_BUF_FRAMES  BEEP_FRAMES(PLAY_SAMPR_MAX, KEYCLICK_DURATION)
static int16_t beep_buf[BEEP_BUF_FRAMES];

/* Callback to generate the beep frames - also don't want inlining of
   call below in beep_play */
static NO_INLINE int
beep_get_more(int status, const void **start, unsigned long *frames)
{
    if (!PCM_STATUS_CONTINUABLE(status))
        return status;

    unsigned long rem = beep_rem;

    if (rem)
    {
        rem = MIN(rem, BEEP_BUF_FRAMES);
        beep_rem -= rem;
        *start = beep_buf;
        *frames = rem;
        beep_generate(beep_buf, rem, &beep_phase, beep_step);
    }

    return 0;
}

/* Generates a constant square wave sound with a given frequency in Hertz for
   a duration in milliseconds */
void beep_play(unsigned int frequency, unsigned int duration,
               unsigned int amplitude)
{
    if (pcm_handle)
        pcm_stop(pcm_handle);
    else if (!(pcm_handle = pcm_open(PCM_STREAM_PLAYBACK)))
        return;

    if (frequency == 0 || duration == 0 || amplitude == 0)
        return;

    /* Setup the parameters for the square wave generator */
    uint32_t fout = pcm_get_frequency(pcm_handle);
    beep_phase = 0;
    beep_step = fp_div(frequency, fout, 32);
    beep_rem = BEEP_FRAMES(fout, duration);

    /* If it fits - avoid cb overhead */
    const void *start;
    unsigned long count;

    /* Generate first frame here */
    beep_get_more(0, &start, &count);

    if (amplitude > INT16_MAX)
        amplitude = INT16_MAX;

    pcm_set_amplitude(pcm_handle, amplitude*PCM_AMP_UNITY / INT16_MAX);

    pcm_play_data(pcm_handle, beep_rem ? beep_get_more : NULL,
                  start, count, PCM_FORMAT_T_PARM(PCM_FORMAT_S16_1CH_2I));
}
