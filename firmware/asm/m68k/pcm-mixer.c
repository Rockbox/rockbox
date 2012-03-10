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

#define MIXER_OPTIMIZED_MIX_SAMPLES
#define MIXER_OPTIMIZED_WRITE_SAMPLES
static struct emac_context
{
    unsigned long saved;
    unsigned long r[4];
} emac_context IBSS_ATTR;

/* Save emac context affected in ISR */
static FORCE_INLINE void save_emac_context(void)
{
    /* Save only if not already saved */
    if (emac_context.saved == 0)
    {
        emac_context.saved = 1;
        asm volatile (
            "move.l   %%macsr, %%d0             \n"
            "move.l   %%accext01, %%d1          \n"
            "movclr.l %%acc0, %%a0              \n"
            "movclr.l %%acc1, %%a1              \n"
            "movem.l  %%d0-%%d1/%%a0-%%a1, (%0) \n"
            "move.l   %1, %%macsr               \n"
            :
            : "a"(&emac_context.r), "i"(EMAC_ROUND | EMAC_SATURATE)
            : "d0", "d1", "a0", "a1");
    }
}

/* Restore emac context affected in ISR */
static FORCE_INLINE void restore_emac_context(void)
{
    /* Restore only if saved */
    if (UNLIKELY(emac_context.saved != 0))
    {
        asm volatile (
           "movem.l (%0), %%d0-%%d1/%%a0-%%a1  \n"
           "move.l  %%a1, %%acc1               \n"
           "move.l  %%a0, %%acc0               \n"
           "move.l  %%d1, %%accext01           \n"
           "move.l  %%d0, %%macsr              \n"
           :
           : "a"(&emac_context.r)
           : "d0", "d1", "a0", "a1");
        emac_context.saved = 0;
    }
}

#define mixer_buffer_callback_exit() restore_emac_context()

/* Mix channels' samples and apply gain factors */
static FORCE_INLINE void mix_samples(void *out,
                                     const void *src0,
                                     int32_t src0_amp,
                                     const void *src1,
                                     int32_t src1_amp,
                                     size_t size)
{
    uint32_t s0, s1, s2, s3;
    save_emac_context();

    asm volatile (
        "move.l     (%1)+, %5                 \n"
    "1:                                       \n"
        "movea.w    %5, %4                    \n"
        "asr.l      %10, %5                   \n"
        "mac.l      %4, %8,            %%acc0 \n"
        "mac.l      %5, %8, (%2)+, %5, %%acc1 \n"
        "movea.w    %5, %4                    \n"
        "asr.l      %10, %5                   \n"
        "mac.l      %4, %9,            %%acc0 \n"
        "mac.l      %5, %9, (%1)+, %5, %%acc1 \n"
        "movclr.l   %%acc0, %6                \n"
        "movclr.l   %%acc1, %7                \n"
        "swap.w     %6                        \n"
        "move.w     %6, %7                    \n"
        "move.l     %7, (%0)+                 \n"
        "subq.l     #4, %3                    \n"
        "bhi.b      1b                        \n"
        : "+a"(out), "+a"(src0), "+a"(src1), "+d"(size),
          "=&a"(s0), "=&d"(s1), "=&d"(s2), "=&d"(s3)
        : "r"(src0_amp), "r"(src1_amp), "d"(16)
    );
}

/* Write channel's samples and apply gain factor */
static FORCE_INLINE void write_samples(void *out,
                                       const void *src,
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
        uint32_t s0, s1, s2, s3;
        save_emac_context();

        asm volatile (
            "move.l     (%1)+, %4                 \n"
        "1:                                       \n"
            "movea.w    %4, %3                    \n"
            "asr.l      %8, %4                    \n"
            "mac.l      %3, %7,            %%acc0 \n"
            "mac.l      %4, %7, (%1)+, %4, %%acc1 \n"
            "movclr.l   %%acc0, %5                \n"
            "movclr.l   %%acc1, %6                \n"
            "swap.w     %5                        \n"
            "move.w     %5, %6                    \n"
            "move.l     %6, (%0)+                 \n"
            "subq.l     #4, %2                    \n"
            "bhi.b      1b                        \n"
            : "+a"(out), "+a"(src), "+d"(size),
              "=&a"(s0), "=&d"(s1), "=&d"(s2), "=&d"(s3)
            : "r"(amp), "d"(16)
        );
    }     
}
