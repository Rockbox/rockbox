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

static int32_t beep_phase;      /* Phase of square wave generator */
static uint32_t beep_step;      /* Step of square wave generator on each sample */
static uint32_t beep_amplitude; /* Amplitude of square wave generator */
static int beep_count;          /* Number of samples remaining to generate */

/* Reserve enough static space for keyclick to fit */
#define BEEP_BUF_COUNT (NATIVE_FREQUENCY / 1000 * KEYCLICK_DURATION)
static uint32_t beep_buf[BEEP_BUF_COUNT] IBSS_ATTR;

/* Actually output samples into beep_buf */
#if defined(CPU_ARM)
static FORCE_INLINE void beep_generate(int count)
{
    uint32_t *out = beep_buf;
    uint32_t s;

    asm volatile (
    "1:                            \n"
        "eor   %3, %5, %1, asr #31 \n"
        "subs  %2, %2, #1          \n"
        "str   %3, [%0], #4        \n"
        "add   %1, %1, %4          \n"
        "bgt   1b                  \n"
        : "+r"(out), "+r"(beep_phase), "+r"(count),
          "=&r"(s)
        : "r"(beep_step), "r"(beep_amplitude));
}
#elif defined (CPU_COLDFIRE)
static FORCE_INLINE void beep_generate(int count)
{
    uint32_t *out = beep_buf;
    uint32_t s;

    asm volatile (
    "1:                   \n"
        "move.l %1, %3    \n"
        "add.l  %4, %1    \n"
        "add.l  %3, %3    \n"
        "subx.l %3, %3    \n"
        "eor.l  %5, %3    \n"
        "move.l %3, (%0)+ \n"
        "subq.l #1, %2    \n"
        "bgt.b  1b        \n"
        : "+a"(out), "+d"(beep_phase), "+d"(count),
          "=&d"(s)
        : "r"(beep_step), "d"(beep_amplitude));
}
#else
static FORCE_INLINE void beep_generate(int count)
{
    uint32_t *out = beep_buf;
    uint32_t amplitude = beep_amplitude;
    uint32_t step = beep_step;
    int32_t phase = beep_phase;

    do
    {
        *out++ = (phase >> 31) ^ amplitude;
        phase += step;
    }
    while (--count > 0);

    beep_phase = phase;
}
#endif

/* Callback to generate the beep frames - also don't want inlining of
   call below in beep_play */
static void __attribute__((noinline))
beep_get_more(unsigned char **start, size_t *size)
{
    int count = beep_count;

    if (count > 0)
    {
        count = MIN(count, BEEP_BUF_COUNT);
        beep_count -= count;
        *start = (unsigned char *)beep_buf;
        *size = count * sizeof(uint32_t);
        beep_generate(count);
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
    beep_phase = 0;
    beep_step = 0xffffffffu / NATIVE_FREQUENCY * frequency;
    beep_count = NATIVE_FREQUENCY / 1000 * duration;
    beep_amplitude = amplitude | (amplitude << 16); /* Word:|AMP16|AMP16| */

    /* If it fits - avoid cb overhead */
    unsigned char *start;
    size_t size;

    /* Generate first frame here */
    beep_get_more(&start, &size);

    mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    mixer_channel_play_data(PCM_MIXER_CHAN_BEEP,
                            beep_count ? beep_get_more : NULL,
                            start, size);
}
