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

#define MIXER_OPTIMIZED_WRITE_SAMPLES
#define MIXER_OPTIMIZED_MIX_SAMPLES

/* Mix channels' samples and apply gain factors */
static FORCE_INLINE void mix_samples(void *out,
                                     const void *src0,
                                     int32_t src0_amp,
                                     const void *src1,
                                     int32_t src1_amp,
                                     size_t size)
{
    if (src0_amp == MIX_AMP_UNITY && src1_amp == MIX_AMP_UNITY)
    {
        /* Both are unity amplitude */
        int32_t l0, l1, h0, h1;
        asm volatile (
        "1:                             \n"
            "ldrsh  %4, [%1], #2        \n"
            "ldrsh  %5, [%2], #2        \n"
            "ldrsh  %6, [%1], #2        \n"
            "ldrsh  %7, [%2], #2        \n"
            "add    %4, %4, %5          \n"
            "add    %6, %6, %7          \n"
            "mov    %5, %4, asr #15     \n"
            "teq    %5, %5, asr #31     \n"
            "eorne  %4, %8, %4, asr #31 \n"
            "mov    %7, %6, asr #15     \n"
            "teq    %7, %7, asr #31     \n"
            "eorne  %6, %8, %6, asr #31 \n"
            "subs   %3, %3, #4          \n"
            "and    %4, %4, %8, lsr #16 \n"
            "orr    %6, %4, %6, lsl #16 \n"
            "str    %6, [%0], #4        \n"
            "bhi    1b                  \n"
            : "+r"(out), "+r"(src0), "+r"(src1), "+r"(size),
              "=&r"(l0), "=&r"(l1), "=&r"(h0), "=&r"(h1)
            : "r"(0xffff7fff));
    }
    else if (src0_amp != MIX_AMP_UNITY && src1_amp != MIX_AMP_UNITY)
    {
        /* Neither are unity amplitude */
        int32_t l0, l1, h0, h1;
        asm volatile (
        "1:                              \n"
            "ldrsh  %4, [%1], #2         \n"
            "ldrsh  %5, [%2], #2         \n"
            "ldrsh  %6, [%1], #2         \n"
            "ldrsh  %7, [%2], #2         \n"
            "mul    %4, %8, %4           \n"
            "mul    %5, %9, %5           \n"
            "mul    %6, %8, %6           \n"
            "mul    %7, %9, %7           \n"
            "mov    %4, %4, asr #16      \n"
            "add    %4, %4, %5, asr #16  \n"
            "mov    %6, %6, asr #16      \n"
            "add    %6, %6, %7, asr #16  \n"
            "mov    %5, %4, asr #15      \n"
            "teq    %5, %5, asr #31      \n"
            "eorne  %4, %10, %4, asr #31 \n"
            "mov    %7, %6, asr #15      \n"
            "teq    %7, %7, asr #31      \n"
            "eorne  %6, %10, %6, asr #31 \n"
            "subs   %3, %3, #4           \n"
            "and    %4, %4, %10, lsr #16 \n"
            "orr    %6, %4, %6, lsl #16  \n"
            "str    %6, [%0], #4         \n"
            "bhi    1b                   \n"
            : "+r"(out), "+r"(src0), "+r"(src1), "+r"(size),
              "=&r"(l0), "=&r"(l1), "=&r"(h0), "=&r"(h1)
            : "r"(src0_amp), "r"(src1_amp), "r"(0xffff7fff));
    }
    else
    {
        /* One is unity amplitude */
        if (src0_amp != MIX_AMP_UNITY)
        {
            /* Keep unity in src0, amp0 */
            const void *src_tmp = src0;
            src0 = src1;
            src1 = src_tmp;
            src1_amp = src0_amp;
            src0_amp = MIX_AMP_UNITY;
        }

        int32_t l0, l1, h0, h1;
        asm volatile (
        "1:                             \n"
            "ldrsh  %4, [%1], #2        \n"
            "ldrsh  %5, [%2], #2        \n"
            "ldrsh  %6, [%1], #2        \n"
            "ldrsh  %7, [%2], #2        \n"
            "mul    %5, %8, %5          \n"
            "mul    %7, %8, %7          \n"
            "add    %4, %4, %5, asr #16 \n"
            "add    %6, %6, %7, asr #16 \n"
            "mov    %5, %4, asr #15     \n"
            "teq    %5, %5, asr #31     \n"
            "eorne  %4, %9, %4, asr #31 \n"
            "mov    %7, %6, asr #15     \n"
            "teq    %7, %7, asr #31     \n"
            "eorne  %6, %9, %6, asr #31 \n"
            "subs   %3, %3, #4          \n"
            "and    %4, %4, %9, lsr #16 \n"
            "orr    %6, %4, %6, lsl #16 \n"
            "str    %6, [%0], #4        \n"
            "bhi    1b                  \n"
            : "+r"(out), "+r"(src0), "+r"(src1), "+r"(size),
              "=&r"(l0), "=&r"(l1), "=&r"(h0), "=&r"(h1)
            : "r"(src1_amp), "r"(0xffff7fff));
    }
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
        asm volatile (
            "ands       r1, %2, #0x1f  \n"
            "beq        2f             \n"
        "1:                            \n"
            "ldr        r0, [%1], #4   \n"
            "subs       r1, r1, #4     \n"
            "str        r0, [%0], #4   \n"
            "bne        1b             \n"
            "bics       %2, %2, #0x1f  \n"
            "beq        3f             \n"
        "2:                            \n"
            "ldmia      %1!, { r0-r7 } \n"
            "subs       %2, %2, #32    \n"
            "stmia      %0!, { r0-r7 } \n"
            "bhi        2b             \n"
        "3:                            \n"
            : "+r"(out), "+r"(src), "+r"(size)
            :
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7");
    }
    else
    {
        /* Channel needs amplitude cut */
        uint32_t l, h;
        asm volatile (
        "1:                              \n"
            "ldrsh   %3, [%1], #2        \n"
            "ldrsh   %4, [%1], #2        \n"
            "subs    %2, %2, #4          \n"
            "mul     %3, %5, %3          \n"
            "mul     %4, %5, %4          \n"
            "and     %4, %4, %6, lsl #16 \n"
            "orr     %4, %4, %3, lsr #16 \n"
            "str     %4, [%0], #4        \n"
            "bhi     1b                  \n"
            : "+r"(out), "+r"(src), "+r"(size),
              "=&r"(l), "=&r"(h)
            : "r"(amp), "r"(0xffffffffu));
    }     
}
