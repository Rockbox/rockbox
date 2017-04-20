/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Michael Sevakis
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

/* Functions compatible with all ARMv4+ */

static void __attribute__((naked))
write_samples_unity_1ch_s16_2(struct pcm_stream_desc *desc,
                              void *out,
                              unsigned long count)
{
    asm volatile (
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "ands    r3, r2, #0x7             \n"
        "beq     2f                       \n"
    "1:  ldrh    r12, [r0], #2            \n" /* first 1 to 7 frames */
        "subs    r3, r3, #1               \n"
        "orr     r12, r12, r12, lsl #16   \n"
        "str     r12, [r1], #4            \n"
        "bne     1b                       \n"
        "bics    r2, r2, #0x7             \n"
        "bxeq    lr                       \n"
    "2:  tst     r0, #2                   \n" /* word-aligned? */
        "stmeqfd sp!, { r4-r9 }           \n"
        "stmnefd sp!, { r4-r10 }          \n"
        "bne     4f                       \n"
    "3:  ldmia   r0!, { r3, r5, r7, r9 }  \n" /* 8-chunk, word-aligned */
        "subs    r2, r2, #8               \n"
        "mov     r4, r3, lsr #16          \n" /* M1.M1 */
        "orr     r4, r4, r4, lsl #16      \n"
        "mov     r3, r3, lsl #16          \n" /* M0.M0 */
        "orr     r3, r3, r3, lsr #16      \n"
        "mov     r6, r5, lsr #16          \n" /* M3.M3 */
        "orr     r6, r6, r6, lsl #16      \n"
        "mov     r5, r5, lsl #16          \n" /* M2.M2 */
        "orr     r5, r5, r5, lsr #16      \n"
        "mov     r8, r7, lsr #16          \n" /* M5.M5 */
        "orr     r8, r8, r8, lsl #16      \n"
        "mov     r7, r7, lsl #16          \n" /* M4.M4 */
        "orr     r7, r7, r7, lsr #16      \n"
        "mov     r12, r9, lsr #16         \n" /* M7.M7 */
        "orr     r12, r12, r12, lsl #16   \n"
        "mov     r9, r9, lsl #16          \n" /* M6.M6 */
        "orr     r9, r9, r9, lsr #16      \n"
        "stmia   r1!, { r3-r9, r12 }      \n"
        "bhi     3b                       \n"
        "ldmfd   sp!, { r4-r9 }           \n"
        "bx      lr                       \n"
    "4:  ldrh    r3, [r0], #2             \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia   r0!, { r5, r7, r9, r12 } \n"
        "subs    r2, r2, #8               \n"
        "orr     r4, r3, r3, lsl #16      \n" /* M0.M0 */
        "mov     r6, r5, lsr #16          \n" /* M2.M2 */
        "orr     r6, r6, r6, lsl #16      \n"
        "mov     r5, r5, lsl #16          \n" /* M1.M1 */
        "orr     r5, r5, r5, lsr #16      \n"
        "mov     r8, r7, lsr #16          \n" /* M4.M4 */
        "orr     r8, r8, r8, lsl #16      \n"
        "mov     r7, r7, lsl #16          \n" /* M3.M3 */
        "orr     r7, r7, r7, lsr #16      \n"
        "mov     r10, r9, lsr #16         \n" /* M6.M6 */
        "orr     r10, r10, r10, lsl #16   \n"
        "mov     r9, r9, lsl #16          \n" /* M5.M5 */
        "orr     r9, r9, r9, lsr #16      \n"
        "movhi   r3, r12, lsr #16         \n" /* M8?... => M0 for next loop */
        "mov     r12, r12, lsl #16        \n" /* M7.M7 */
        "orr     r12, r12, r12, lsr #16   \n"
        "stmia   r1!, { r4-r10, r12 }     \n"
        "bhi     5b                       \n"
        "ldmfd   sp!, { r4-r10 }          \n"
        "bx      lr                       \n"
    :
    : "i"(PCM_STR_DESC_ADDR_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
write_samples_unity_2ch_s16_2(struct pcm_stream_desc *desc,
                              void *out,
                              unsigned long count)
{
    asm volatile (
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "ands    r3, r2, #0x7             \n"
        "beq     2f                       \n"
    "1:  ldr     r12, [r0], #4            \n" /* first 1 to 7 frames */
        "subs    r3, r3, #1               \n"
        "str     r12, [r1], #4            \n"
        "bne     1b                       \n"
        "bics    r2, r2, #0x7             \n"
        "bxeq    lr                       \n"
    "2:  tst     r0, #2                   \n" /* word aligned? */
        "stmeqfd sp!, { r4-r9 }           \n"
        "stmnefd sp!, { r4-r10 }          \n"
        "ldrneh  r3, [r0], #2             \n"
        "bne     5f                       \n"
    "3:  ldmia   r0!, { r3-r9, r12 }      \n" /* 8-chunk, word-aligned */
        "subs    r2, r2, #8               \n"
        "stmia   r1!, { r3-r9, r12 }      \n"
        "bhi     3b                       \n"
        "ldmfd   sp!, { r4-r9 }           \n"
        "bx      lr                       \n"
    "4:  mov     r3, r12, lsr #16         \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia   r0!, { r4-r10, r12 }     \n"
        "subs    r2, r2, #8               \n"
        "orr     r3, r3, r4, lsl #16      \n" /* L0.R0 */
        "mov     r4, r4, lsr #16          \n" /* L1.R1 */
        "orr     r4, r5, r5, lsl #16      \n"
        "mov     r5, r5, lsr #16          \n" /* L2.R2 */
        "orr     r5, r5, r6, lsl #16      \n"
        "mov     r6, r6, lsr #16          \n" /* L3.R3 */
        "orr     r6, r6, r7, lsl #16      \n"
        "mov     r7, r7, lsr #16          \n" /* L4.R4 */
        "orr     r7, r7, r8, lsl #16      \n"
        "mov     r8, r8, lsr #16          \n" /* L5.R5 */
        "orr     r8, r8, r9, lsl #16      \n"
        "mov     r9, r9, lsr #16          \n" /* L6.R6 */
        "orr     r9, r9, r10, lsl #16     \n"
        "mov     r10, r10, lsr #16        \n" /* L7.R7 */
        "orr     r10, r10, r12, lsl #16   \n"
        "stmia   r1!, { r3-r10 }          \n"
        "bhi     4b                       \n"
        "ldmfd   sp!, { r4-r10 }          \n"
        "bx      lr                       \n"
    :
    : "i"(PCM_STR_DESC_ADDR_OFFS));

    (void)out; (void)desc; (void)count;
}
