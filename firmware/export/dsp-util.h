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
static FORCE_INLINE int32_t clip_sample_arm_6(int32_t sample,
                                              unsigned int bits)
{
    if (bits < 32) {
        int32_t out;

        if (__builtin_constant_p(bits)) {
            asm ("ssat  %0, %2, %1          \n"
                : "=r" (out)
                : "r"(sample), "i"(bits));
        }
        else {
            int shift = 32 - bits;
            asm ("mov   %0, %2, asl %3      \n"
                 "teq   %2, %0, asr %3      \n"
                 "eorne %0, %1, %2, asr #31 \n"
                : "=&r"(out)
                : "r"(0x7ffffffful >> shift), "r"(sample), "r"(shift));
        }

        return out;
    }
    else {
        return sample; /* can't be out of range */
    }
}

static FORCE_INLINE int32_t mix_clip_sample_arm_6(int32_t sample1,
                                                  int32_t sample2,
                                                  unsigned int bits)
{
    if (bits < 32) {
        /* add then saturate */
        return clip_sample_arm_6(sample1 + sample2, bits);
    }
    else {
        /* add and saturate in one */
        int32_t out;
        asm ("qadd  %0, %1, %2"
        : "=r" (out)
        : "r"(sample1), "r"(sample2));
        return out;
    }
}

static FORCE_INLINE int32_t read_int24_preidx_arm_6(void **pp,
                                                    int preidx,
                                                    bool writeback)
{
    int32_t out;

    if (writeback) {
        if (__builtin_constant_p(preidx)) {
            asm ("ldr   %0, [%1, %2]!      \n"
                : "=&r" (out), "+r"(*pp) : "i"(3*preidx));

        }
        else {
            asm ("add,  %1, %1, %2, asl #1 \n"
                 "ldr   %0, [%1, %2]!      \n"
                : "=&r" (out), "+r"(*pp) : "r"(preidx));
        }
    }
    else {
        if (__builtin_constant_p(preidx)) {
            asm ("ldr   %0, [%1, %2]       \n"
                : "=&r" (out) : "r"(*pp), "i"(3*preidx));

        }
        else {
            asm ("add,  %1, %1, %2, asl #1 \n"
                 "ldr   %0, [%1, %2]       \n"
                : "=&r" (out) : "r"(*pp), "r"(preidx));
        }
    }

    return (out << 8) >> 8;
}

#define clip_sample         clip_sample_arm_6
#define mix_clip_sample     mix_clip_sample_arm_6
#define read_int24_preidx   read_int24_preidx_arm_6

#else /* ARM_ARCH < 6 */

#if 0
#define clip_sample     clip_sample_arm
#define mix_clip_sample mix_clip_sample_arm
#define read_int24_preidx   read_int24_preidx_arm
#endif

#endif /* ARM_ARCH */
#elif defined (CPU_COLDFIRE)

#endif /* CPU_* */

/* Generic, possibly overridden above */

#ifndef clip_sample
static FORCE_INLINE int32_t clip_sample(int32_t sample,
                                        unsigned int bits)
{
    if (bits < 32) {
        int shift = 32 - bits;
        if (((sample << shift) >> shift) != sample) {
            sample = (0x7ffffffful >> shift) ^ (sample >> 31);
        }
    }
    /* else can't be out of range */

    return sample;
}
#endif /* clip_sample */

static FORCE_INLINE int32_t clip_sample_shl(int32_t sample,
                                            unsigned int shift,
                                            unsigned int inbits)
{
    return clip_sample(sample, inbits) << shift;
}

static FORCE_INLINE int32_t clip_sample_shr(int32_t sample,
                                            unsigned int shift,
                                            unsigned int outbits,
                                            int32_t dc_bias)
{
    return clip_sample((sample + dc_bias) >> shift, outbits);
}

#ifndef mix_clip_sample
static FORCE_INLINE int32_t mix_clip_sample(int32_t sample1,
                                            int32_t sample2,
                                            unsigned int bits)
{
    if (bits < 32) {
        return clip_sample(sample1 + sample2, bits);
    }
    else {
        int64_t out = (int64_t)sample1 + sample2;
        if ((int32_t)out != out) {
            out = 0x7ffffffful ^ (out >> 63);
        }
        return out;
    }
}
#endif /* mix_clip_sample */

static FORCE_INLINE int32_t amplify_sample(int32_t sample,
                                           int32_t factor,
                                           unsigned int depth,
                                           unsigned int fracbits)
{
    int32_t round = 1L << (fracbits - 1);

    if (depth + fracbits <= 32) {
        return (factor * sample + round) >> fracbits;
    }
    else {
        return ((int64_t)factor * sample + round) >> fracbits;
    }
}

#define clip_sample_16(sample) \
    clip_sample((sample), 16)

#define mix_clip_sample_16(sample1, sample2) \
    mix_clip_sample((sample1), (sample2), 16)

#define pcm_dma_t_clip(sample) \
    clip_sample((sample), PCM_DMA_T_DEPTH)

#define pcm_dma_t_mix_clip(sample1, sample2) \
    mix_clip_sample((sample1), (sample2), PCM_DMA_T_DEPTH)

#define pcm_dma_t_amplify(sample, factor, fracbits) \
    amplify_sample((sample), (factor), PCM_DMA_T_DEPTH, (fracbits))

/* Absolute difference of signed 32-bit numbers which must be dealt with
 * in the unsigned 32-bit range */
static FORCE_INLINE uint32_t ad_s32(int32_t a, int32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

#undef CLIP_SAMPLE_DEFINED


static FORCE_INLINE int32_t read_int32_preidx(const void **pp,
                                              int preidx,
                                              bool writeback)
{
    const int32_t *p = *pp;
    int32_t s = *(p + preidx);

    if (writeback) {
        *pp = p;
    }

    return s;
}

#ifndef read_int24_preidx
static FORCE_INLINE int32_t read_int24_preidx(const void **pp,
                                              int preidx,
                                              bool writeback)
{
    const uint8_t *p = *pp + 3*preidx;
    int32_t s;

#ifdef ROCKBOX_BIG_ENDIAN
    s = ((int32_t)((int8_t *)p)[0] << 16) | ((int32_t)p[1] << 8) | ((int32_t)p[2]);
#else
    s = ((int32_t)p[0]) | ((int32_t)p[1] << 8) | ((int32_t)((int8_t *)p)[2] << 16);
#endif

    if (writeback) {
        *pp = p;
    }

    return s;
}
#endif /* read_int24_preidx */

static FORCE_INLINE int16_t read_int16_preidx(const void **pp,
                                              int preidx,
                                              bool writeback)
{
    const int16_t *p = *pp;
    int32_t s = *(p + preidx);

    if (writeback) {
        *pp = p;
    }

    return s;
}

#endif /* DSP_UTIL_H */
