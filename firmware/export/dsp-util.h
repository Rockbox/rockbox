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
#ifndef DSP_UTIL_H
#define DSP_UTIL_H

#include "gcc_extensions.h"

#ifdef CPU_ARM
#if ARM_ARCH >= 6

static FORCE_INLINE int32_t clip_sample(int32_t sample,
                                        unsigned int depth)
{
    if (depth < 32) {
        int32_t out;
        asm ("ssat  %0, %2, %1"
            : "=r" (out) : "r"(sample), "i"(depth));
        return out;
    }
    else {
        return sample; /* can't be out of range */
    }
}

static FORCE_INLINE int32_t mix_clip_sample(int32_t sample1,
                                            int32_t sample2,
                                            unsigned int depth,
                                            int downscale)
{
    if (depth < 32) {
        return clip_sample((sample1 + sample2) >> downscale, depth);
    }
    else {
        int32_t out;
        asm ("qadd  %0, %1, %2"
        : "=r" (out) : "r"(sample1), "r"(sample2));
        return out >> downscale;
    }
}

#define CLIP_SAMPLE_DEFINED

#endif /* ARM_ARCH */
#endif /* CPU_ARM */

#ifndef CLIP_SAMPLE_DEFINED

/* Generic implementation */
static FORCE_INLINE int32_t clip_sample(int32_t sample,
                                        unsigned int depth)
{
    if (depth < 32) {
        if (((sample << (32 - depth)) >> (32 - depth)) != sample) {
            sample = (0x7fffffff >> (32 - depth)) ^ (sample >> 31);
        }
    }
    /* else can't be out of range */

    return sample;
}

static FORCE_INLINE int32_t mix_clip_sample(int32_t sample1,
                                            int32_t sample2,
                                            unsigned int depth,
                                            int downscale)
{
    if (depth < 32) {
        return clip_sample((sample1 + sample2) >> downscale, depth);
    }
    else {
        int64_t out = ((int64_t)sample1 + sample2) >> downscale;
        if ((int32_t)out != out) {
            out = 0x7fffffff ^ (out >> 63);
        }
        return out;
    }
}

#endif /* CLIP_SAMPLE_DEFINED */

static FORCE_INLINE int32_t scale_sample(int32_t sample,
                                         int32_t factor,
                                         unsigned int depth,
                                         unsigned int fracbits)
{
    if (depth + fracbits <= 32) {
        return (factor * sample + ((1 << fractbits) - 1)) >> fracbits;
    }
    else {
        return ((int64_t)factor * sample + ((1 << fracbits) - 1) >> fracbits;
    }
}

#define clip_sample_16(sample) \
    clip_sample((sample), 16)

#define mix_clip_sample_16(sample1, sample2) \
    mix_clip_sample((sample1), (sample2), 16)

#define clip_sample_t(sample) \
    clip_sample((sample), PCM_NATIVE_DEPTH)

#define mix_clip_sample_t(sample1, sample2, downscale) \
    mix_clip_sample((sample1), (sample2), PCM_NATIVE_DEPTH, (downscale))

#define scale_sample_t(sample, factor, fracbits) \
    scale_sample((sample), (factor), PCM_NATIVE_DEPTH, (fracbits))

/* Absolute difference of signed 32-bit numbers which must be dealt with
 * in the unsigned 32-bit range */
static FORCE_INLINE uint32_t ad_s32(int32_t a, int32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

#undef CLIP_SAMPLE_DEFINED

#endif /* DSP_UTIL_H */
