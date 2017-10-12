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

/* Functions compatible with all ARMv4+ */

#ifndef write_samples_unity_1ch_s16i_s16i
#define write_samples_unity_1ch_s16i_s16i write_samples_unity_1ch_s16i_s16i_arm4

static void __attribute__((naked))
write_samples_unity_1ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                       void *out,
                                       unsigned long count)
{
    asm volatile (
        "ldr     r0, [r0, %0]             \n" /* r0 = pcm->addr */
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
    : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_unity_1ch_s16i_s16i */

#ifndef write_samples_unity_2ch_s16i_s16i
#define write_samples_unity_2ch_s16i_s16i write_samples_unity_2ch_s16i_s16i_arm4

static void __attribute__((naked))
write_samples_unity_2ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                       void *out,
                                       unsigned long count)
{
    asm volatile (
        "ldr     r0, [r0, %0]             \n" /* r0 = pcm->addr */
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
        "orr     r3, r3, r4, lsl #16      \n" /* R0.L0 */
        "mov     r4, r4, lsr #16          \n" /* R1.L1 */
        "orr     r4, r5, r5, lsl #16      \n"
        "mov     r5, r5, lsr #16          \n" /* R2.L2 */
        "orr     r5, r5, r6, lsl #16      \n"
        "mov     r6, r6, lsr #16          \n" /* R3.L3 */
        "orr     r6, r6, r7, lsl #16      \n"
        "mov     r7, r7, lsr #16          \n" /* R4.L4 */
        "orr     r7, r7, r8, lsl #16      \n"
        "mov     r8, r8, lsr #16          \n" /* R5.L5 */
        "orr     r8, r8, r9, lsl #16      \n"
        "mov     r9, r9, lsr #16          \n" /* R6.L6 */
        "orr     r9, r9, r10, lsl #16     \n"
        "mov     r10, r10, lsr #16        \n" /* R7.L7 */
        "orr     r10, r10, r12, lsl #16   \n"
        "stmia   r1!, { r3-r10 }          \n"
        "bhi     4b                       \n"
        "ldmfd   sp!, { r4-r10 }          \n"
        "bx      lr                       \n"
    :
    : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_unity_2ch_s16i_s16i */

#ifndef write_samples_gain_1ch_s16i_s16i
#define write_samples_gain_1ch_s16i_s16i write_samples_gain_1ch_s16i_s16i_arm4

static void __attribute__((naked))
write_samples_gain_1ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                      void *out,
                                      unsigned long count)
{
    asm volatile(
        "str     r4, [sp, #-4]!           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude_tx */
        "ldr     r0, [r0, %0]             \n" /* r0 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* SM0 */
        "ldrsh   r12, [r0], #2            \n" /* SM1 */
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
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_gain_1ch_s16i_s16i */

#ifndef write_samples_gain_2ch_s16i_s16i
#define write_samples_gain_2ch_s16i_s16i write_samples_gain_2ch_s16i_s16i_arm4

static void __attribute__((naked))
write_samples_gain_2ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                      void *out,
                                      unsigned long count)
{
    asm volatile(
        "stmfd   sp!, { r4-r6 }           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude_tx */
        "ldr     r0, [r0, %0]             \n" /* r0 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* SL0 */
        "ldrsh   r5, [r0], #2             \n" /* SR0 */
        "ldrsh   r6, [r0], #2             \n" /* SL1 */
        "ldrsh   r12, [r0], #2            \n" /* SR1 */
        "subs    r2, r2, #2               \n"
        "mul     r4, r3, r4               \n" /* A*SL0 */
        "mul     r5, r3, r5               \n" /* A*SR0 */
        "mul     r6, r3, r6               \n" /* A*SL1 */
        "mul     r12, r3, r12             \n" /* A*SR1 */
        "mov     r4, r4, lsr #16          \n" /* A*SR0.A*SL0 */
        "mov     r5, r5, lsr #16          \n"
        "orr     r4, r4, r5, lsl #16      \n"
        "mov     r5, r6, lsr #16          \n" /* A*SR1.A*SL1 */
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
        "mov     r4, r4, lsr #16          \n" /* A*SR0.A*SL0 */
        "mov     r5, r5, lsr #16          \n"
        "orr     r4, r4, r5, lsl #16      \n"
        "str     r4, [r1]                 \n"
        "ldmfd   sp!, { r4-r6 }           \n"
        "bx      lr                       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_gain_2ch_s16i_s16i */

#ifndef mix_samples_unity_1ch_s16i_s16i
#define mix_samples_unity_1ch_s16i_s16i mix_samples_unity_1ch_s16i_s16i_arm4

static void __attribute__((naked))
mix_samples_unity_1ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                     void *out,
                                     unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4-r8 }           \n"
        "ldr     r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldmia   r1, { r6-r7  }           \n" /* DR0.DL0|DR1.DL1 */
        "ldrsh   r3, [r0], #2             \n" /* SM0 */
        "ldrsh   r4, [r0], #2             \n" /* SM1 */
        "mov     r5, r6, asl #16          \n" /* DL0<<16 */
        "add     r6, r3, r6, asr #16      \n" /* DR0+SM0 */
        "add     r5, r3, r5, asr #16      \n" /* DL0+SM0 */
        "mov     r3, r7, asl #16          \n" /* DL1<<16 */
        "add     r7, r4, r7, asr #16      \n" /* DR1+SM1 */
        "add     r3, r4, r3, asr #16      \n" /* DL1+SM1 */
        "mov     r4, r6, asr #15          \n" /* SAT[SM0+DR0] */
        "teq     r4, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r4, r5, asr #15          \n" /* SAT[SM0+DL0] */
        "teq     r4, r5, asr #31          \n"
        "eorne   r5, r12, r5, asr #31     \n"
        "mov     r4, r7, asr #15          \n" /* SAT[SM1+DR1] */
        "teq     r4, r7, asr #31          \n"
        "eorne   r7, r12, r7, asr #31     \n"
        "mov     r4, r3, asr #15          \n" /* SAT[SM1+DL1] */
        "teq     r4, r3, asr #31          \n"
        "eorne   r3, r12, r3, asr #31     \n"
        "and     r5, r5, r12, lsr #16     \n" /* SM0+DR0.SM0+DL0 */
        "orr     r4, r5, r6, lsl #16      \n"
        "and     r3, r3, r12, lsr #16     \n" /* SM1+DR1.SM1+DL1 */
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
        "and     r5, r5, r12, lsr #16     \n" /* SM0+DR0.SM0+DL0 */
        "orr     r4, r5, r6, lsl #16      \n"
        "str     r4, [r1]                 \n"
        "ldmfd   sp!, { r4-r8 }           \n"
        "bx      lr                       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_unity_1ch_s16i_s16i */

#ifndef mix_samples_unity_2ch_s16i_s16i
#define mix_samples_unity_2ch_s16i_s16i mix_samples_unity_2ch_s16i_s16i_arm4

static void __attribute__((naked))
mix_samples_unity_2ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                     void *out,
                                     unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4-r9 }           \n"
        "ldr     r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r3, [r0], #2             \n" /* SL0 */
        "ldrsh   r4, [r0], #2             \n" /* SR0 */
        "ldmia   r1, { r8-r9  }           \n" /* DR0.DL0|DR1.DL1 */
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
        "and     r3, r3, r12, lsr #16     \n" /* SR0+DR0.SL0+DL0 */
        "orr     r7, r3, r4, lsl #16      \n"
        "and     r5, r5, r12, lsr #16     \n" /* SR1+DR1.SL1+DL1 */
        "orr     r8, r5, r6, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r7-r8 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r9 }           \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldr     r8, [r1]                 \n" /* DR0.DL0 */
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
        "and     r3, r3, r12, lsr #16     \n" /* SR0+DR0.SL0+DL0 */
        "orr     r7, r3, r4, lsl #16      \n"
        "str     r7, [r1]                 \n"
        "ldmfd   sp!, { r4-r9 }           \n"
        "bx      lr                       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_unity_2ch_s16i_s16i */

#ifndef mix_samples_gain_1ch_s16i_s16i
#define mix_samples_gain_1ch_s16i_s16i mix_samples_gain_1ch_s16i_s16i_arm4

static void __attribute__((naked))
mix_samples_gain_1ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                    void *out,
                                    unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4-r8 }           \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = desc->amplitude_tx */
        "ldr     r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* SM0 */
        "ldrsh   r5, [r0], #2             \n" /* SM1 */
        "ldmia   r1, { r7-r8  }           \n" /* DR0.DL0|DR1.DL1 */
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
        "and     r4, r4, r12, lsr #16     \n" /* A*M0+DR0.A*M0+DL0 */
        "orr     r6, r4, r6, lsl #16      \n"
        "and     r5, r5, r12, lsr #16     \n" /* A*M1+DR1.A*M1+DL1 */
        "orr     r7, r5, r7, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r6-r7 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r8 }           \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldrsh   r4, [r0]                 \n" /* SM0 */
        "ldr     r7, [r1]                 \n" /* DR0.DL0 */
        "mul     r4, r3, r4               \n" /* A*SM0 */
        "mov     r6, r7, asr #16          \n" /* DR0 */
        "mov     r7, r7, asl #16          \n" /* DL0 */
        "mov     r7, r7, asr #16          \n"
        "add     r6, r6, r4, asr #16      \n" /* DR0+SM0 */
        "add     r4, r7, r4, asr #16      \n" /* DL0+SM0 */
        "mov     r8, r6, asr #15          \n" /* SAT[SM0+DR0] */
        "teq     r8, r6, asr #31          \n"
        "eorne   r6, r12, r6, asr #31     \n"
        "mov     r8, r4, asr #15          \n" /* SAT[SM0+DL0] */
        "teq     r8, r4, asr #31          \n"
        "eorne   r4, r12, r4, asr #31     \n"
        "and     r4, r4, r12, lsr #16     \n" /* A*SM0+DR0.A*SM0+DL0 */
        "orr     r6, r4, r6, lsl #16      \n"
        "str     r6, [r1]                 \n"
        "ldmfd   sp!, { r4-r8 }           \n"
        "bx      lr                       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_gain_1ch_s16i_s16i */

#ifndef mix_samples_gain_2ch_s16i_s16i
#define mix_samples_gain_2ch_s16i_s16i mix_samples_gain_2ch_s16i_s16i_arm4

static void __attribute__((naked))
mix_samples_gain_2ch_s16i_s16i_arm4(struct pcm_stream_desc *desc,
                                    void *out,
                                    unsigned long count)
{
    asm volatile (
        "stmfd   sp!, { r4-r10 }          \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = desc->amplitude_tx */
        "ldr     r0, [r0, %0]             \n" /* r1 = desc->addr_tx */
        "subs    r2, r2, #1               \n"
        "mvn     r12, #0x00008000         \n" /* 0xffff7fff */
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldrsh   r4, [r0], #2             \n" /* SL0 */
        "ldrsh   r5, [r0], #2             \n" /* SR0 */
        "ldrsh   r6, [r0], #2             \n" /* SL1 */
        "ldrsh   r7, [r0], #2             \n" /* SR1 */
        "ldmia   r1, { r9-r10  }          \n" /* DR0.DL0|DR1.DL1 */
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
        "and     r4, r4, r12, lsr #16     \n" /* A*SR0+DR0.A*SL0+DL0 */
        "orr     r8, r4, r5, lsl #16      \n"
        "and     r6, r6, r12, lsr #16     \n" /* A*SR1+DR1.A*SL1+DL1 */
        "orr     r9, r6, r7, lsl #16      \n"
        "subs    r2, r2, #2               \n"
        "stmia   r1!, { r8-r9 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r10 }          \n" /* event count? return */
        "bxlo    lr                       \n"
    "2:  ldrsh   r4, [r0, #0]             \n" /* SL0 */
        "ldrsh   r5, [r0, #2]             \n" /* SR0 */
        "ldr     r9, [r1]                 \n" /* DR0.DL0 */
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
        "and     r4, r4, r12, lsr #16     \n" /* A*SR0+DR0.A*SL0+DL0 */
        "orr     r8, r4, r5, lsl #16      \n"
        "str     r8, [r1]                 \n"
        "ldmfd   sp!, { r4-r10 }          \n"
        "bx      lr                       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_gain_2ch_s16i_s16i */
