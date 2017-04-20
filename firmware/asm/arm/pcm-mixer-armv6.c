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
        "pkhbt   r12, r12, r12, asl #16   \n"
        "str     r12, [r1], #4            \n"
        "bne     1b                       \n"
        "bics    r2, r2, #0x7             \n"
        "bxeq    lr                       \n"
    "2:  tst     r0, #2                   \n" /* word-aligned? */
        "stmeqfd sp!, { r4-r8, lr }       \n"
        "stmnefd sp!, { r4-r9, lr }       \n"
        "bne     4f                       \n"
    "3:  ldmia   r0!, { r3, r5, r7, r12 } \n" /* 8-chunk, word-aligned */
        "subs    r2, r2, #8               \n"
        "pkhtb   r4, r3, r3, asr #16      \n" /* M1.M1 */
        "pkhbt   r3, r3, r3, asl #16      \n" /* M0.M0 */
        "pkhtb   r6, r5, r5, asr #16      \n" /* M3.M3 */
        "pkhbt   r5, r5, r5, asl #16      \n" /* M2.M2 */
        "pkhtb   r8, r7, r7, asr #16      \n" /* M5.M5 */
        "pkhbt   r7, r7, r7, asl #16      \n" /* M4.M4 */
        "pkhtb   r14, r12, r12, asr #16   \n" /* M7.M7 */
        "pkhbt   r12, r12, r12, asl #16   \n" /* M6.M6 */
        "stmia   r1!, { r3-r8, r12, r14 } \n"
        "bhi     3b                       \n"
        "ldmfd   sp!, { r4-r8, pc }       \n"
    "4:  ldrh    r3, [r0], #2             \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia   r0!, { r5, r7, r9, r14 } \n"
        "subs    r2, r2, #8               \n"
        "pkhbt   r4, r3, r3, asl #16      \n" /* M0.M0 */
        "pkhtb   r6, r5, r5, asr #16      \n" /* M2.M2 */
        "pkhbt   r5, r5, r5, asl #16      \n" /* M1.M1 */
        "pkhtb   r8, r7, r7, asr #16      \n" /* M4.M4 */
        "pkhbt   r7, r7, r7, asl #16      \n" /* M3.M3 */
        "pkhtb   r12, r9, r9, asr #16     \n" /* M6.M6 */
        "pkhbt   r9, r9, r9, asl #16      \n" /* M5.M5 */
        "movhi   r3, r14, lsr #16         \n" /* M8?... => M0 for next loop */
        "pkhbt   r14, r14, r14, asl #16   \n" /* M7.M7 */
        "stmia   r1!, { r4-r9, r12, r14 } \n"
        "bhi     5b                       \n"
        "ldmfd   sp!, { r4-r9, pc }       \n"
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
        "stmeqfd sp!, { r4-r8, lr }       \n"
        "stmnefd sp!, { r4-r9, lr }       \n"
        "ldrneh  r3, [r0], #2             \n"
        "bne     5f                       \n"
    "3:  ldmia   r0!, { r3-r8, r12, r14 } \n" /* 8-chunk, word-aligned */
        "subs    r2, r2, #8               \n"
        "stmia   r1!, { r3-r8, r12, r14 } \n"
        "bhi     3b                       \n"
        "ldmfd   sp!, { r4-r8, pc }       \n"
    "4:  mov     r3, r14                  \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia   r0!, { r4-r9, r12, r14 } \n"
        "subs    r2, r2, #8               \n"
        "pkhbt   r3, r3, r4, asl #16      \n" /* L0.R0 */
        "mov     r5, r5, ror #16          \n"
        "pkhtb   r4, r5, r4, asr #16      \n" /* L1.R1 */
        "pkhbt   r5, r5, r6, asl #16      \n" /* L2.R2 */
        "mov     r7, r7, ror #16          \n"
        "pkhtb   r6, r7, r6, asr #16      \n" /* L3.R3 */
        "pkhbt   r7, r7, r8, asl #16      \n" /* L4.R4 */
        "mov     r9, r9, ror #16          \n"
        "pkhtb   r8, r9, r8, asr #16      \n" /* L5.R5 */
        "pkhbt   r9, r9, r12, asl #16     \n" /* L6.R6 */
        "mov     r14, r14, ror #16        \n"
        "pkhtb   r12, r14, r12, asr #16   \n" /* L7.R7 */
        "stmia   r1!, { r3-r9, r12 }      \n"
        "bhi     4b                       \n"
        "ldmfd   sp!, { r4-r9, pc }       \n"
    :
    : "i"(PCM_STR_DESC_ADDR_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
write_samples_gain_1ch_s16_2(struct pcm_stream_desc *desc,
                             void *out,
                             unsigned long count)
{
    asm volatile(
        "str     lr, [sp, #-4]!           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldr     r14, [r0], #4            \n" /* M0.M1 */
        "subs    r2, r2, #2               \n"
        "smulwb  r12, r3, r14             \n" /* M0 * amp */
        "smulwt  r14, r3, r14             \n" /* M1 * amp */
        "pkhbt   r12, r12, r12, asl #16   \n" /* M0.M0 */
        "pkhbt   r14, r14, r14, asl #16   \n" /* M1.M1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldrlo   pc, [sp], #4             \n" /* event count? return */
    "2:  ldrh    r14, [r0]                \n" /* M0 */
        "smulwb  r12, r3, r14             \n" /* M0 * amp */
        "pkhbt   r12, r12, r12, asl #16   \n" /* M0.M0 */
        "str     r12, [r1]                \n"
        "ldr     pc, [sp], #4             \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS), "i"(PCM_STR_DESC_AMP_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
write_samples_gain_2ch_s16_2(struct pcm_stream_desc *desc,
                             void *out,
                             unsigned long count)
{
    asm volatile(
        "stmfd   sp!, { r4-r5, lr }       \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldr     r12, [r0], #4            \n" /* L0.R0 */
        "ldr     r14, [r0], #4            \n" /* L1.R1 */
        "subs    r2, r2, #2               \n"
        "smulwb  r4, r3, r12              \n" /* L0 * amp */
        "smulwt  r12, r3, r12             \n" /* R0 * amp */
        "smulwb  r5, r3, r14              \n" /* L1 * amp */
        "smulwt  r14, r3, r14             \n" /* R1 * amp */
        "pkhbt   r12, r4, r12, asl #16    \n" /* L0.R0 */
        "pkhbt   r14, r5, r14, asl #16    \n" /* L1.R1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r5, pc }       \n" /* event count? return */
    "2:  ldr     r12, [r0]                \n" /* L0.R0 */
        "smulwb  r4, r3, r12              \n" /* L0 * amp */
        "smulwt  r12, r3, r12             \n" /* R0 * amp */
        "pkhbt   r12, r4, r12, asl #16    \n" /* L0.R0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { r4-r5, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS), "i"(PCM_STR_DESC_AMP_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
mix_samples_unity_1ch_s16_2(struct pcm_stream_desc *desc,
                            void *out,
                            unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4, lr }          \n"
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldr     r4, [r0], #4             \n" /* M0.M1 */
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "subs    r2, r2, #2               \n"
        "pkhbt   r3, r4, r4, asl #16      \n" /* M0.M0 */
        "pkhtb   r4, r4, r4, asr #16      \n" /* M1.M1 */
        "qadd16  r12, r3, r12             \n" /* M0+DL0.M0+DR0 */
        "qadd16  r14, r4, r14             \n" /* M1+DL1.M1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldrh    r4, [r0]                 \n" /* M0... */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */
        "pkhbt   r3, r4, r4, asl #16      \n" /* M0.M0 */
        "qadd16  r12, r3, r12             \n" /* M0+DL0.M0+DR0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { r4, pc }          \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
mix_samples_unity_2ch_s16_2(struct pcm_stream_desc *desc,
                            void *out,
                            unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4, lr }          \n"
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "ldr     r3, [r0], #4             \n" /* L0.R0 */
        "ldr     r4, [r0], #4             \n" /* L1.R1 */
        "subs    r2, r2, #2               \n"
        "qadd16  r12, r3, r12             \n" /* L0+DL0.R0+DR0 */
        "qadd16  r14, r4, r14             \n" /* L1+DL1.R1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldr     r12, [r1]                \n" /* DL0.DR0 */
        "ldr     r3, [r0]                 \n" /* L0.R0 */
        "qadd16  r12, r3, r12             \n" /* L0+DL0.R0+DR0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { r4, pc }          \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
mix_samples_gain_1ch_s16_2(struct pcm_stream_desc *desc,
                           void *out,
                           unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4-r5, lr }       \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldr     r5, [r0], #4             \n" /* M0.M1 */
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "subs    r2, r2, #2               \n"
        "smulwb  r4, r3, r5               \n" /* M0 * amp */
        "smulwt  r5, r3, r5               \n" /* M1 * amp */
        "pkhbt   r4, r4, r4, asl #16      \n" /* M0.M0 */
        "pkhbt   r5, r5, r5, asl #16      \n" /* M1.M1 */
        "qadd16  r12, r4, r12             \n" /* M0+DL0.M0+DR0 */
        "qadd16  r14, r5, r14             \n" /* M1+DL1.M1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r5, pc }       \n" /* event count? return */
    "2:  ldrh    r5, [r0]                 \n" /* M0 */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */
        "smulwb  r4, r3, r5               \n" /* M0 * amp */
        "pkhbt   r4, r4, r4, asl #16      \n" /* M0.M0 */
        "qadd16  r12, r4, r12             \n" /* M0+DL0.M0+DR0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { r4-r5, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS), "i"(PCM_STR_DESC_AMP_OFFS));

    (void)out; (void)desc; (void)count;
}

static void __attribute__((naked))
mix_samples_gain_2ch_s16_2(struct pcm_stream_desc *desc,
                           void *out,
                           unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4-r7, lr }       \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldr     r4, [r0], #4             \n" /* L0.R0 */
        "ldr     r6, [r0], #4             \n" /* L1.R1 */
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "subs    r2, r2, #2               \n"
        "smulwb  r5, r3, r4               \n" /* L0 * amp */
        "smulwt  r4, r3, r4               \n" /* R0 * amp */
        "smulwb  r7, r3, r6               \n" /* L1 * amp */
        "smulwt  r6, r3, r6               \n" /* R1 * amp */
        "pkhbt   r4, r5, r4, asl #16      \n" /* L0.R0 */
        "pkhbt   r6, r7, r6, asl #16      \n" /* L1.R1 */
        "qadd16  r12, r4, r12             \n" /* L0+DL0.R0+DR0 */
        "qadd16  r14, r6, r14             \n" /* L1+DL1.R1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r7, pc }       \n" /* even count? return */
    "2:  ldr     r4, [r0]                 \n" /* L0.R0 */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */
        "smulwb  r5, r3, r4               \n" /* L0 * amp */
        "smulwt  r4, r3, r4               \n" /* R0 * amp */
        "pkhbt   r4, r5, r4, asl #16      \n" /* L0.R0 */
        "qadd16  r12, r4, r12             \n" /* L0+DL0.R0+DR0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { r4-r7, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS), "i"(PCM_STR_DESC_AMP_OFFS));

    (void)out; (void)desc; (void)count;
}
