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

static FORCE_INLINE void write_samples(void *out,
                                       const struct pcm_handle *pcm,
                                       unsigned long count)
{
    int32_t amp = pcm->amplitude;
    int16_t const *src = pcm->addr;
    int32_t t0, t1;

    if (LIKELY(amp == MIX_AMP_UNITY)) {
        asm volatile (
            "ands       r1, %2, #0x7   \n"
            "beq        2f             \n"
        "1:                            \n"
            "ldr        r0, [%1], #4   \n"
            "subs       r1, r1, #1     \n"
            "str        r0, [%0], #4   \n"
            "bne        1b             \n"
            "bics       %2, %2, #0x7   \n"
            "beq        3f             \n"
        "2:                            \n"
            "ldmia      %1!, { r0-r7 } \n"
            "subs       %2, %2, #8     \n"
            "stmia      %0!, { r0-r7 } \n"
            "bhi        2b             \n"
        "3:                            \n"
            : "+r"(out), "+r"(src), "+r"(count)
            :
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7");
    }
    else {
        asm volatile (
        "1:                              \n"
            "ldrsh   %3, [%1], #2        \n"
            "ldrsh   %4, [%1], #2        \n"
            "subs    %2, %2, #1          \n"
            "mul     %3, %5, %3          \n"
            "mul     %4, %5, %4          \n"
            "and     %4, %4, %6, lsl #16 \n"
            "orr     %3, %4, %3, lsr #16 \n"
            "str     %3, [%0], #4        \n"
            "bhi     1b                  \n"
            : "+r"(out), "+r"(src), "+r"(count),
              "=&r"(t0), "=&r"(t1)
            : "r"(amp), "r"(0xffffffffu));
    }
}

static FORCE_INLINE void mix_samples(void *out,
                                     const struct pcm_handle *pcm,
                                     unsigned long count)
{
    int32_t amp = pcm->amplitude;
    int16_t const *src = pcm->addr;
    int32_t t0, t1, t2, t3;

    if (amp == MIX_AMP_UNITY) {
        asm volatile (
        "1:                             \n"
            "ldrsh  %3, [%0, #0]        \n"
            "ldrsh  %4, [%0, #2]        \n"
            "ldrsh  %5, [%1], #2        \n"
            "ldrsh  %6, [%1], #2        \n"
            "add    %5, %5, %3          \n"
            "add    %6, %6, %4          \n"
            "mov    %3, %5, asr #15     \n"
            "teq    %3, %5, asr #31     \n"
            "eorne  %3, %7, %5, asr #31 \n"
            "mov    %4, %6, asr #15     \n"
            "teq    %4, %6, asr #31     \n"
            "eorne  %4, %7, %6, asr #31 \n"
            "subs   %2, %2, #1          \n"
            "and    %3, %3, %7, lsr #16 \n"
            "orr    %3, %3, %4, lsl #16 \n"
            "str    %3, [%0], #4        \n"
            "bhi    1b                  \n"
            : "+r"(out), "+r"(src), "+r"(count),
              "=&r"(t0), "=&r"(t1), "=&r"(t2),  "=&r"(t3)
            : "r"(0xffff7fff));
    }
    else {
        asm volatile (
        "1:                             \n"
            "ldrsh  %3, [%0, #0]        \n"
            "ldrsh  %4, [%0, #2]        \n"
            "ldrsh  %5, [%1], #2        \n"
            "ldrsh  %6, [%1], #2        \n"
            "mul    %5, %7, %5          \n"
            "mul    %6, %7, %6          \n"
            "add    %5, %3, %5, asr #16 \n"
            "add    %6, %4, %6, asr #16 \n"
            "mov    %3, %5, asr #15     \n"
            "teq    %3, %5, asr #31     \n"
            "eorne  %3, %8, %5, asr #31 \n"
            "mov    %4, %6, asr #15     \n"
            "teq    %4, %6, asr #31     \n"
            "eorne  %4, %8, %6, asr #31 \n"
            "subs   %2, %2, #1          \n"
            "and    %3, %3, %8, lsr #16 \n"
            "orr    %3, %3, %4, lsl #16 \n"
            "str    %3, [%0], #4        \n"
            "bhi    1b                  \n"
            : "+r"(out), "+r"(src), "+r"(count),
              "=&r"(t0), "=&r"(t1), "=&r"(t2), "=&r"(t3)
            : "r"(amp), "r"(0xffff7fff));
    }
}
