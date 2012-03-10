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
/* Mix channels' samples and apply gain factors */
static FORCE_INLINE void mix_samples(int16_t *out,
                                     const int16_t *src0,
                                     int32_t src0_amp,
                                     const int16_t *src1,
                                     int32_t src1_amp,
                                     size_t size)
{
    if (src0_amp == MIX_AMP_UNITY && src1_amp == MIX_AMP_UNITY)
    {
        /* Both are unity amplitude */
        do
        {
            int32_t l = *src0++ + *src1++;
            int32_t h = *src0++ + *src1++;
            *out++ = clip_sample_16(l);
            *out++ = clip_sample_16(h);
        }
        while ((size -= 2*sizeof(int16_t)) > 0);
    }
    else if (src0_amp != MIX_AMP_UNITY && src1_amp != MIX_AMP_UNITY)
    {
        /* Neither are unity amplitude */
        do
        {
            int32_t l = (*src0++ * src0_amp >> 16) + (*src1++ * src1_amp >> 16);
            int32_t h = (*src0++ * src0_amp >> 16) + (*src1++ * src1_amp >> 16);
            *out++ = clip_sample_16(l);
            *out++ = clip_sample_16(h);
        }
        while ((size -= 2*sizeof(int16_t)) > 0);
    }
    else
    {
        /* One is unity amplitude */
        if (src0_amp != MIX_AMP_UNITY)
        {
            /* Keep unity in src0, amp0 */
            const int16_t *src_tmp = src0;
            src0 = src1;
            src1 = src_tmp;
            src1_amp = src0_amp;
            src0_amp = MIX_AMP_UNITY;
        }

        do
        {
            int32_t l = *src0++ + (*src1++ * src1_amp >> 16);
            int32_t h = *src0++ + (*src1++ * src1_amp >> 16);
            *out++ = clip_sample_16(l);
            *out++ = clip_sample_16(h);
        }
        while ((size -= 2*sizeof(int16_t)) > 0);
    }
}

/* Write channel's samples and apply gain factor */
static FORCE_INLINE void write_samples(int16_t *out,
                                       const int16_t *src,
                                       int32_t amp,
                                       size_t size)
{
    if (LIKELY(amp == MIX_AMP_UNITY))
    {
        /* Channel is unity amplitude */
        memcpy(out, src, size);
    }
    else
    {
        /* Channel needs amplitude cut */
        do
        {
            int32_t l = *src++ * amp >> 16;
            int32_t h = *src++ * amp >> 16;
            *out++ = l;
            *out++ = h;
        }
        while ((size -= 2*sizeof(int16_t)) > 0);
    }     
}


#endif /* CPU_* */

#ifndef mixer_buffer_callback_exit
#define mixer_buffer_callback_exit() do{}while(0)
#endif
