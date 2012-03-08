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
#include "dsp.h"
#include "pcm.h"
#include "pcm_mixer.h"
#include "misc.h"

/** Beep generation, CPU optimized **/
#include "asm/beep.c"

static uint32_t beep_phase;     /* Phase of square wave generator */
static uint32_t beep_step;      /* Step of square wave generator on each sample */
#ifdef BEEP_GENERIC
static int16_t  beep_amplitude; /* Amplitude of square wave generator */
#else
/* Optimized routines do XOR with phase sign bit in both channels at once */
static uint32_t beep_amplitude; /* Amplitude of square wave generator */
#endif
static int beep_count;          /* Number of samples remaining to generate */

static const struct beep_params *melody_note = NULL;
static int melody_count = 0;

/* Reserve enough static space for keyclick to fit */
#define BEEP_BUF_COUNT (NATIVE_FREQUENCY * (int64_t)KEYCLICK_DURATION / 1000)
static int16_t beep_buf[BEEP_BUF_COUNT*2] IBSS_ATTR __attribute__((aligned(4)));

static void beep_set_params(unsigned int frequency, unsigned int duration,
                            unsigned int amplitude)
{
    /* Setup the parameters for the square wave generator */
    beep_phase = 0;

    if (frequency == 0 || amplitude == 0)
    {
        /* Generating musical rest */
        frequency = amplitude = 0;
    }

    beep_step = (0x100000000ull * frequency / NATIVE_FREQUENCY) >> 16;
    beep_count = NATIVE_FREQUENCY * (uint64_t)duration / 1000000;

#ifdef BEEP_GENERIC
    beep_amplitude = amplitude;
#else
    /* Optimized routines do XOR with phase sign bit in both channels at once */
    beep_amplitude = amplitude | (amplitude << 16); /* Word:|AMP16|AMP16| */
#endif
}

/* Callback to generate the beep frames - also don't want inlining of
   call below in beep_play */
static void __attribute__((noinline))
beep_get_more(const void **start, size_t *size)
{
    while (1)
    {
        int count = beep_count;

        if (LIKELY(count > 0))
        {
            count = MIN(count, BEEP_BUF_COUNT);
            beep_count -= count;
            *start = beep_buf;
            *size = count * 2 * sizeof (int16_t);
            beep_generate((void *)beep_buf, count, &beep_phase,
                          beep_step, beep_amplitude);
            return;
        }

        if (melody_note == NULL)
            return;

        if (--melody_count == 0)
        {
            melody_note = NULL;
            return;
        }

        /* Get next note to play */
        const struct beep_params *note = ++melody_note;
        beep_set_params(note->frequency, note->duration, note->amplitude);
    }
}


/* Generates a constant square wave sound with a given frequency in Hertz for
   a duration in milliseconds */
void beep_play(unsigned int frequency, unsigned int duration,
               unsigned int amplitude)
{
    mixer_channel_stop(PCM_MIXER_CHAN_BEEP);

    melody_note = NULL;
    melody_count = 0;    

    if (frequency == 0 || duration == 0 || amplitude == 0)
        return;

    if (amplitude > INT16_MAX)
        amplitude = INT16_MAX;

    beep_set_params(frequency << 16, duration*1000, amplitude);

    /* If it fits - avoid cb overhead */
    const void *start;
    size_t size;

    /* Generate first frame here */
    beep_get_more(&start, &size);

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    mixer_channel_play_data(PCM_MIXER_CHAN_BEEP,
                            beep_count ? beep_get_more : NULL,
                            start, size);
}

void beep_play_melody(const struct beep_params *params, unsigned int count)
{
    mixer_channel_stop(PCM_MIXER_CHAN_BEEP);

    if (!(params && count > 0))
        return;

    melody_note = params;
    melody_count = count;

    beep_set_params(params->frequency, params->duration, params->amplitude);

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, beep_get_more, NULL, 0);
}
