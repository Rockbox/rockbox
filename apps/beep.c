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
#include "pcm_mixer.h"
#include "misc.h"
#include "fixedpoint.h"

/** Beep generation, CPU optimized **/
#include "asm/beep.c"

static uint32_t beep_phase;      /* Phase of square wave generator */
static uint32_t beep_step;       /* Step of square wave generator on each sample */
static unsigned long beep_rem;   /* Number of frames remaining to generate */

#define BEEP_FRAMES(fs, duration) ((fs) / 1000 * (duration))

/* Reserve enough static space for keyclick to fit in worst case */
#define BEEP_BUF_FRAMES  BEEP_FRAMES(PLAY_SAMPR_MAX, KEYCLICK_DURATION)
static int16_t beep_buf[BEEP_BUF_FRAMES*2] IBSS_ATTR ALIGNED_ATTR(4);

/* Callback to generate the beep frames - also don't want inlining of
   call below in beep_play */
static NO_INLINE void
beep_get_more(const void **start, unsigned long *frames)
{
    unsigned long rem = beep_rem;

    if (rem)
    {
        rem = MIN(rem, BEEP_BUF_FRAMES);
        beep_rem -= rem;
        *start = beep_buf;
        *frames = rem;
        beep_generate(beep_buf, rem, &beep_phase, beep_step);
    }
}

/* Generates a constant square wave sound with a given frequency in Hertz for
   a duration in milliseconds */
void beep_play(unsigned int frequency, unsigned int duration,
               unsigned int amplitude)
{
    mixer_channel_stop(PCM_MIXER_CHAN_BEEP);

    if (frequency == 0 || duration == 0 || amplitude == 0)
        return;

    if (amplitude > INT16_MAX)
        amplitude = INT16_MAX;

    /* Setup the parameters for the square wave generator */
    uint32_t fout = mixer_get_frequency();
    beep_phase = 0;
    beep_step = fp_div(frequency, fout, 32);
    beep_rem = BEEP_FRAMES(fout, duration);

    /* If it fits - avoid cb overhead */
    const void *start;
    unsigned long count;

    /* Generate first frame here */
    beep_get_more(&start, &count);

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP,
                                amplitude*MIX_AMP_UNITY / INT16_MAX);

    mixer_channel_play_data(PCM_MIXER_CHAN_BEEP,
                            beep_rem ? beep_get_more : NULL,
                            start, count);
}
