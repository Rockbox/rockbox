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

/* write_samples_unity_1ch_s16_2 */
/* write_samples_unity_2ch_s16_2 */
#include "pcm-mixer-arm-generic.c"

static void __attribute__((naked))
write_samples_gain_1ch_s16_2(struct pcm_stream_desc *desc,
                             void *out,
                             unsigned long count)
{
    asm volatile(
        "str     r4, [sp, #-4]!           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* M0 */
        "ldrsh   r12, [r0], #2            \n" /* M1 */
        "subs    r2, r2, #2               \n"
        "mul     r4, r3, r4               \n" /* A*SM0 */
        "mul     r12, r3, r12             \n" /* A*SM1 */
        "mov     r4, r4, lsr #16          \n" /* A*SM0.A*SM0 */
        "orr     r4, r4, r4, lsl #16      \n"
        "mov     r12, r12, lsr #16        \n" /* A*SM1.A*SM1 */
        "orr     r12, r12, r12, lsl #16   \n"
        "stmia   r1!, { r4, r12 }         \n"
        "bhi     1b                       \n"
        "ldrlo   r4, [sp], #4             \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldrsh   r4, [r0]                 \n" /* M0 */
        "mul     r4, r3, r4               \n" /* A*SM0 */
        "mov     r4, r4, lsl #16          \n" /* A*SM0.A*SM0 */
        "orr     r4, r4, r4, lsr #16      \n"
        "str     r4, [r1]                 \n"
        "ldr     r4, [sp], #4             \n"
        "bx      lr                       \n"
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
        "stmfd   sp!, { r4-r6 }           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* L0 */
        "ldrsh   r5, [r0], #2             \n" /* R0 */
        "ldrsh   r6, [r0], #2             \n" /* L1 */
        "ldrsh   r12, [r0], #2            \n" /* R1 */
        "subs    r2, r2, #2               \n"
        "mul     r4, r3, r4               \n" /* A*SL0 */
        "mul     r5, r3, r5               \n" /* A*SR0 */
        "mul     r6, r3, r6               \n" /* A*SL1 */
        "mul     r12, r3, r12             \n" /* A*SR1 */
        "mov     r4, r4, lsr #16          \n" /* L0.R0 */
        "mov     r5, r5, lsr #16          \n"
        "orr     r4, r4, r5, lsl #16      \n"
        "mov     r5, r6, lsr #16          \n" /* L1.R1 */
        "mov     r12, r12, lsr #16        \n"
        "orr     r5, r5, r12, lsl #16     \n"
        "stmia   r1!, { r4-r5 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r6 }           \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldrsh   r4, [r0, #0]             \n" /* L0 */
        "ldrsh   r5, [r0, #2]             \n" /* R0 */
        "mul     r4, r3, r4               \n" /* A*SL0 */
        "mul     r5, r3, r5               \n" /* A*SR0 */
        "mov     r4, r4, lsr #16          \n" /* L0.R0 */
        "mov     r5, r5, lsr #16          \n"
        "orr     r4, r4, r5, lsl #16      \n"
        "str     r4, [r1]                 \n"
        "ldmfd   sp!, { r4-r6 }           \n"
        "bx      lr                       \n"
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
        "stmfd   sp!, { r4-r8 }           \n"
        "ldr     r0, [r0, %0]             \n" /* r1 = desc->addr */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldmia   r1, { r6-r7  }           \n" /* DL0.DR0|DL1.DR1 */
        "ldrsh   r3, [r0], #2             \n" /* MS0 */
        "ldrsh   r4, [r0], #2             \n" /* MS1 */
        "mov     r5, r6, asl #16          \n" /* DL0<<16 */
        "add     r6, r3, r6, asr #16      \n" /* DR0+MS0 */
        "add     r5, r3, r5, asr #16      \n" /* DL0+MS0 */
        "mov     r3, r7, asl #16          \n" /* DL1<<16 */
        "add     r7, r4, r7, asr #16      \n" /* DR1+MS1 */
        "add     r3, r4, r3, asr #16      \n" /* DL1+MS1 */
        "mov     r4, r6, asr #15          \n" /* SAT[M0+DR0] */
        "teq     r4, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r4, r5, asr #15          \n" /* SAT[MS0+DL0] */
        "teq     r4, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "mov     r4, r7, asr #15          \n" /* SAT[M1+DR1] */
        "teq     r4, r7, asr #31          \n"
        "eorne   r7, r12, r7, asr #31     \n"
        "mov     r4, r3, asr #15          \n" /* SAT[M1+DL1] */
        "teq     r4, r3, asr #31          \n"
        "eorne   r3, r12, r3, asr #31     \n"
        "and     r5, r5, r12, lsr #16     \n" /* OL0.OR0 */
        "orr     r4, r5, r6, lsl #16      \n"
        "and     r3, r3, r12, lsr #16     \n" /* OL1.OR1 */
        "orr     r5, r3, r7, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r4-r5 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r8 }           \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldr     r6, [r1]                 \n" /* DL0.DR0 */
        "ldrsh   r3, [r0]                 \n" /* SM0 */
        "mov     r5, r6, asl #16          \n"
        "add     r6, r3, r6, asr #16      \n" /* DR0+SM0 */
        "add     r5, r3, r5, asr #16      \n" /* DL0+SM0 */
        "mov     r4, r6, asr #15          \n" /* SAT[DL0+SM0] */
        "teq     r4, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r4, r5, asr #15          \n" /* SAT[DR0+SM0] */
        "teq     r4, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "and     r5, r5, r12, lsr #16     \n" /* OL0.OR0 */
        "orr     r4, r5, r6, lsl #16      \n"
        "str     r4, [r1]                 \n"
        "ldmfd   sp!, { r4-r8 }           \n"
        "bx      lr                       \n"
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
        "stmfd   sp!, { r4-r9 }           \n"
        "ldr     r0, [r0, %0]             \n" /* r1 = desc->addr */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r3, [r0], #2             \n" /* SL0 */
        "ldrsh   r4, [r0], #2             \n" /* SR0 */
        "ldmia   r1, { r8-r9  }           \n" /* DL0.DR0|DL1.DR1 */
        "ldrsh   r5, [r0], #2             \n" /* SL1 */
        "ldrsh   r6, [r0], #2             \n" /* SR1 */
        "mov     r7, r8, asl #16          \n" /* DL0 */
        "add     r4, r4, r8, asr #16      \n" /* DR0+SR0 */
        "add     r3, r3, r7, asr #16      \n" /* DL0+SL0 */
        "mov     r8, r9, asl #16          \n" /* DL1 */
        "add     r6, r6, r9, asr #16      \n" /* DR1+SR1 */
        "add     r5, r5, r8, asr #16      \n" /* DL1+SL1 */
        "mov     r7, r4, asr #15          \n" /* SAT[SR0+DR0] */
        "teq     r7, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "mov     r7, r3, asr #15          \n" /* SAT[SL0+DL0] */
        "teq     r7, r3, asr #31          \n"
        "eorne   r3, r12, r3, asr #31     \n"
        "mov     r7, r6, asr #15          \n" /* SAT[SR1+DR1] */
        "teq     r7, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r7, r5, asr #15          \n" /* SAT[SL1+DL1] */
        "teq     r7, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "and     r3, r3, r12, lsr #16     \n" /* LOUT0.ROUT0 */
        "orr     r7, r3, r4, lsl #16      \n"
        "and     r5, r5, r12, lsr #16     \n" /* LOUT1.ROUT1 */
        "orr     r8, r5, r6, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r7-r8 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r9 }           \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldr     r8, [r1]                 \n" /* DL0.DR0 */
        "ldrsh   r3, [r0, #0]             \n" /* SL0 */
        "ldrsh   r4, [r0, #2]             \n" /* SR0 */
        "mov     r7, r8, asl #16          \n" /* DR0 */
        "add     r4, r4, r8, asr #16      \n" /* DR0+SR0 */
        "add     r3, r3, r7, asr #16      \n" /* DL0+SL0 */
        "mov     r7, r4, asr #15          \n" /* SAT[SR0+DR0] */
        "teq     r7, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "mov     r7, r3, asr #15          \n" /* SAT[SL0+DL0] */
        "teq     r7, r3, asr #31          \n"
        "eorne   r3, r12, r3, asr #31     \n"
        "and     r3, r3, r12, lsr #16     \n" /* LO0.RO0 */
        "orr     r7, r3, r4, lsl #16      \n"
        "str     r7, [r1]                 \n"
        "ldmfd   sp!, { r4-r9 }           \n"
        "bx      lr                       \n"
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
        "stmfd   sp!, { r4-r8 }           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = desc->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = desc->addr */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* M0 */
        "ldrsh   r5, [r0], #2             \n" /* M1 */
        "ldmia   r1, { r7-r8  }           \n" /* DL0.DR0|DL1.DR1 */
        "mul     r4, r3, r4               \n" /* A*SM0 */
        "mul     r5, r3, r5               \n" /* A*SM1 */
        "mov     r6, r7, asr #16          \n" /* DR0 */
        "mov     r7, r7, asl #16          \n" /* DL0 */
        "mov     r7, r7, asr #16          \n"
        "add     r6, r6, r4, asr #16      \n" /* DR0+A*SM0 */
        "add     r4, r7, r4, asr #16      \n" /* DL0+A*SM0 */
        "mov     r7, r8, asr #16          \n" /* DR1 */
        "mov     r8, r8, asl #16          \n" /* DL1 */
        "mov     r8, r8, asr #16          \n"
        "add     r7, r7, r5, asr #16      \n" /* DR1+A*SM1 */
        "add     r5, r8, r5, asr #16      \n" /* DL1+A*SM1 */
        "mov     r8, r6, asr #15          \n" /* SAT[A*SM0+DR0] */
        "teq     r8, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r8, r4, asr #15          \n" /* SAT[A*SM0+DL0] */
        "teq     r8, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "mov     r8, r7, asr #15          \n" /* SAT[A*SM1+DR1] */
        "teq     r8, r7, asr #31          \n"
        "eorne   r7, r12, r7, asr #31     \n"
        "mov     r8, r5, asr #15          \n" /* SAT[A*SM1+DL1] */
        "teq     r8, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "and     r4, r4, r12, lsr #16     \n" /* L0.R0 */
        "orr     r6, r4, r6, lsl #16      \n"
        "and     r5, r5, r12, lsr #16     \n" /* L1.R1 */
        "orr     r7, r5, r7, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r6-r7 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r8 }           \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldrsh   r4, [r0]                 \n" /* M0 */
        "ldr     r7, [r1]                 \n" /* DL0.DR0 */
        "mul     r4, r3, r4               \n" /* A*SM0 */
        "mov     r6, r7, asr #16          \n" /* DR0 */
        "mov     r7, r7, asl #16          \n" /* DL0 */
        "mov     r7, r7, asr #16          \n"
        "add     r6, r6, r4, asr #16      \n" /* DR0+M0 */
        "add     r4, r7, r4, asr #16      \n" /* DL0+M0 */
        "mov     r8, r6, asr #15          \n" /* SAT[M0+DR0] */
        "teq     r8, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r8, r4, asr #15          \n" /* SAT[M0+DL0] */
        "teq     r8, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "and     r4, r4, r12, lsr #16     \n" /* L0.R0 */
        "orr     r6, r4, r6, lsl #16      \n"
        "str     r6, [r1]                 \n"
        "ldmfd   sp!, { r4-r8 }           \n"
        "bx      lr                       \n"
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
        "stmfd   sp!, { r4-r10 }          \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = desc->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = desc->addr */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* L0 */
        "ldrsh   r5, [r0], #2             \n" /* R0 */
        "ldrsh   r6, [r0], #2             \n" /* L1 */
        "ldrsh   r7, [r0], #2             \n" /* R1 */
        "ldmia   r1, { r9-r10  }          \n" /* DL0.DR0|DL1.DR1 */
        "mov     r8, r9, asr #16          \n" /* DR0 */
        "mov     r9, r9, asl #16          \n" /* DL0 */
        "mov     r9, r9, asr #16          \n"
        "mul     r4, r3, r4               \n" /* A*SL0 */
        "mul     r5, r3, r5               \n" /* A*SR0 */
        "mul     r6, r3, r6               \n" /* A*SL0 */
        "mul     r7, r3, r7               \n" /* A*SR0 */
        "add     r5, r8, r5, asr #16      \n" /* DR0+A*SR0 */
        "add     r4, r9, r4, asr #16      \n" /* DL0+A*SL0 */
        "mov     r8, r10, asr #16         \n" /* DR1 */
        "mov     r9, r10, asl #16         \n" /* DL1 */
        "mov     r9, r9, asr #16          \n"
        "add     r7, r8, r7, asr #16      \n" /* DR1+A*SR1 */
        "add     r6, r9, r6, asr #16      \n" /* DL1+A*SL1 */
        "mov     r8, r4, asr #15          \n" /* SAT[A*SL0+DL0] */
        "teq     r8, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "mov     r8, r5, asr #15          \n" /* SAT[A*SR0+DR0] */
        "teq     r8, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "mov     r8, r6, asr #15          \n" /* SAT[A*SL1+DL1] */
        "teq     r8, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r8, r7, asr #15          \n" /* SAT[A*SR1+DR1] */
        "teq     r8, r7, asr #31          \n"
        "eorne   r7, r12, r7, asr #31     \n"
        "and     r4, r4, r12, lsr #16     \n" /* LOUT0.ROUT0 */
        "orr     r8, r4, r5, lsl #16      \n"
        "and     r6, r6, r12, lsr #16     \n" /* LOUT1.ROUT1 */
        "orr     r9, r6, r7, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r8-r9 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r10 }          \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldrsh   r4, [r0, #0]             \n" /* L0 */
        "ldrsh   r5, [r0, #2]             \n" /* R0 */
        "ldr     r9, [r1]                 \n" /* DL0.DR0 */
        "mul     r4, r3, r4               \n" /* A*SL0 */
        "mul     r5, r3, r5               \n" /* A*SR0 */
        "mov     r8, r9, asr #16          \n" /* DR0 */
        "mov     r9, r9, asl #16          \n" /* DL0 */
        "mov     r9, r9, asr #16          \n"
        "add     r5, r8, r5, asr #16      \n" /* DR0+A*SR0 */
        "add     r4, r9, r4, asr #16      \n" /* DL0+A*SL0 */
        "mov     r8, r5, asr #15          \n" /* SAT[A*SR0+DR0] */
        "teq     r8, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "mov     r8, r4, asr #15          \n" /* SAT[A*SL0+DL0] */
        "teq     r8, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "and     r4, r4, r12, lsr #16     \n" /* LOUT0.ROUT0 */
        "orr     r8, r4, r5, lsl #16      \n"
        "str     r8, [r1]                 \n"
        "ldmfd   sp!, { r4-r10 }          \n"
        "bx      lr                       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS), "i"(PCM_STR_DESC_AMP_OFFS));

    (void)out; (void)desc; (void)count;
}

#if 0
/* these are the same for unity and gain applied */
#define mix_samples_unity_1ch_s16_2     mix_samples_1ch_s16_2
#define mix_samples_gain_1ch_s16_2      mix_samples_1ch_s16_2
#define mix_samples_unity_2ch_s16_2     mix_samples_2ch_s16_2
#define mix_samples_gain_2ch_s16_2      mix_samples_2ch_s16_2
#endif
