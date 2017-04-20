/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Michael Sevakis
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

#if defined(CPU_ARM)
  #include "arm/pcm-mixer.c"
#elif defined(CPU_COLDFIRE)
  #include "m68k/pcm-mixer.c"
#else

#include "dsp-util.h" /* for clip_sample_16 */

static void mix_samples_mono(void *out,
                             const struct mixer_channel *chan,
                             unsigned long count)
{
    pcm_dma_t *dst = out;
    int32_t amp = chan->amp;

    if (amp == PCM_AMP_UNITY)
    {
        do
        {
            int32_t l = pcm_dma_t_read(dst, 0);
            int32_t r = pcm_dma_t_read(dst, 1);
            int32_t s = *src++;
            l = mix_clip_sample_t(l, s);
            r = mix_clip_sample_t(r, s);
            pcm_dma_t_write_incr(&dst, l, 1);
            pcm_dma_t_write_incr(&dst, r, 1);
        }
        while (--count);
    }
    else
    {
    }
}

static FORCE_INLINE void mix_samples(void *out,
                                     const struct mixer_channel *chan,
                                     unsigned long count)
{
    pcm_dma_t *dst = out;
    int16_t const *src = chan->start;
    int32_t amp = chan->amp;

    if (amp == PCM_AMP_UNITY)
    {

        do
        {
            int32_t l = pcm_dma_t_read(dst, 0);
            int32_t r = pcm_dma_t_read(dst, 1);
            l = mix_clip_sample_t(l, *src++);
            r = mix_clip_sample_t(r, *src++);
            pcm_dma_t_write_incr(&dst, l, 1);
            pcm_dma_t_write_incr(&dst, r, 1);
        }
        while (--count);
    }
    else
    {
        do
        {
            int32_t l = pcm_dma_t_read(dst, 0);
            int32_t r = pcm_dma_t_read(dst, 1);
            l += (*src++ * amp + (1L << (PCM_AMP_FRACBITS-1))) >> PCM_AMP_FRACBITS;
            r += (*src++ * amp + (1L << (PCM_AMP_FRACBITS-1))) >> PCM_AMP_FRACBITS;
            pcm_dma_t_write_incr(&dst, l, 1);
            pcm_dma_t_write_incr(&dst, r, 1);
        }
        while (--count);
    }
}

static void write_samples(void *out,
                          const struct pcm_handle *pcm,
                          unsigned long count)
{
    int32_t amp = chan->amp;
    pcm_dma_t *dst = out;

#if PCM_NATIVE_DEPTH > 16
    if (UNLIKELY(chan->depth < PCM_NATIVE_DEPTH))
    {
        /* Other depth: conversion required */
        int16_t const *src = chan->desc.start;
        int scale = PCM_NATIVE_DEPTH - chan->depth;

        if (LIKELY(amp == MIX_AMP_UNITY))
        {
            /* Channel is unity amplitude */
            do
            {
                *dst++ = *src++ << scale;
                *dst++ = *src++ << scale;
            }
            while (--count);
        }
        else
        {
            /* Channel needs amplitude cut */
            do
            {
                int32_t l = *src++;
                int32_t r = *src++;
                l = (l * amp + (1L << (MIX_AMP_FRACBITS-1))) >> PCM_AMP_FRACBITS;
                r = (r * amp + (1L << (MIX_AMP_FRACBITS-1))) >> PCM_AMP_FRACBITS;
                *dst++ = l << scale;
                *dst++ = r << scale;
            }
            while (--count);
        }
    }
    else
#endif /* PCM_NATIVE_DEPTH > 16 */
    {
        /* Native depth */
        if (LIKELY(amp == MIX_AMP_UNITY))
        {
            /* Channel is unity amplitude */
            memcpy(dst, chan->desc.start, pcm_dma_t_frames_size(count));
        }
        else
        {
            /* Channel needs amplitude cut */
            do
            {
                int32_t l = pcm_dma_t_read_incr(&src, 1);
                int32_t r = pcm_dma_t_read_incr(&src, 1);
                l = scale_sample_t(l, amp, PCM_AMP_FRACBITS);
                r = scale_sample_t(r, amp, PCM_AMP_FRACBITS);
                pcm_dma_t_write_incr(&out, l, 1);
                pcm_dma_t_write_incr(&out, r, 1);
            }
            while (--count);
        }
    }

    (void)depth;
}

static int set_sample_format(const struct pcm_mixer_chan *chan,
                             xxxxxxxxxxxxxxxxx format)
{
    int rc = 0;

    switch (format)
    {
    default:
        rc = -EINVAL;
    }

    return rc;
}

#endif /* CPU_* */

#ifndef mixer_buffer_callback_exit
#define mixer_buffer_callback_exit() do{}while(0)
#endif
