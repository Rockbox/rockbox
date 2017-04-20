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
        "subs    r3, r3, #1               \n" /* SM0 */
        "pkhbt   r12, r12, r12, asl #16   \n" /* OM0.OM0 */
        "str     r12, [r1], #4            \n"
        "bne     1b                       \n"
        "bics    r2, r2, #0x7             \n"
        "bxeq    lr                       \n"
    "2:  tst     r0, #2                   \n" /* word-aligned? */
        "stmeqfd sp!, { r4-r8, lr }       \n"
        "stmnefd sp!, { r4-r9, lr }       \n"
        "bne     4f                       \n"
    "3:  ldmia   r0!, { r3, r5, r7, r12 } \n" /* 8-chunk, word-aligned */
        "subs    r2, r2, #8               \n" /* SM0.SM1|SM2.SM3|SM4.SM5|SM6.SM7 */
        "pkhtb   r4, r3, r3, asr #16      \n" /* OM1.OM1 */
        "pkhbt   r3, r3, r3, asl #16      \n" /* OM0.OM0 */
        "pkhtb   r6, r5, r5, asr #16      \n" /* OM3.OM3 */
        "pkhbt   r5, r5, r5, asl #16      \n" /* OM2.OM2 */
        "pkhtb   r8, r7, r7, asr #16      \n" /* OM5.OM5 */
        "pkhbt   r7, r7, r7, asl #16      \n" /* OM4.OM4 */
        "pkhtb   r14, r12, r12, asr #16   \n" /* OM7.OM7 */
        "pkhbt   r12, r12, r12, asl #16   \n" /* OM6.OM6 */
        "stmia   r1!, { r3-r8, r12, r14 } \n"
        "bhi     3b                       \n"
        "ldmfd   sp!, { r4-r8, pc }       \n"
    "4:  ldrh    r3, [r0], #2             \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia   r0!, { r5, r7, r9, r14 } \n" /* SM0...|SM1.SM2|SM3.SM4|SM5.SM6|SM7.SM8? */
        "subs    r2, r2, #8               \n"
        "pkhbt   r4, r3, r3, asl #16      \n" /* OM0.OM0 */
        "pkhtb   r6, r5, r5, asr #16      \n" /* OM2.OM2 */
        "pkhbt   r5, r5, r5, asl #16      \n" /* OM1.OM1 */
        "pkhtb   r8, r7, r7, asr #16      \n" /* OM4.OM4 */
        "pkhbt   r7, r7, r7, asl #16      \n" /* OM3.OM3 */
        "pkhtb   r12, r9, r9, asr #16     \n" /* OM6.OM6 */
        "pkhbt   r9, r9, r9, asl #16      \n" /* OM5.OM5 */
        "movhi   r3, r14, lsr #16         \n" /* SM8?... => SM0 for next loop */
        "pkhbt   r14, r14, r14, asl #16   \n" /* OM7.OM7 */
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
        "subs    r2, r2, #8               \n" /* SL0.SR0|SL1.SR1|SL2.SR2|SL3.SR3| ... */
        "stmia   r1!, { r3-r8, r12, r14 } \n" /* ... |SL4.SL4|SL5.SR5|SL6.SR6|SL7.SL7 */
        "bhi     3b                       \n"
        "ldmfd   sp!, { r4-r8, pc }       \n"
    "4:  mov     r3, r14                  \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia   r0!, { r4-r9, r12, r14 } \n" /* SL0...|SR0.SL1|SR1.SL2|SR2.SL3|SR3.SL4| ... */
        "subs    r2, r2, #8               \n" /* ... |SR4.SL5|SR5.SL6|SR6.SL7|SR7.SL8?| */
        "pkhbt   r3, r3, r4, asl #16      \n" /* OL0.OR0 */
        "mov     r5, r5, ror #16          \n"
        "pkhtb   r4, r5, r4, asr #16      \n" /* OL1.OR1 */
        "pkhbt   r5, r5, r6, asl #16      \n" /* OL2.OR2 */
        "mov     r7, r7, ror #16          \n"
        "pkhtb   r6, r7, r6, asr #16      \n" /* OL3.OR3 */
        "pkhbt   r7, r7, r8, asl #16      \n" /* OL4.OR4 */
        "mov     r9, r9, ror #16          \n"
        "pkhtb   r8, r9, r8, asr #16      \n" /* OL5.OR5 */
        "pkhbt   r9, r9, r12, asl #16     \n" /* OL6.OR6 */
        "mov     r14, r14, ror #16        \n"
        "pkhtb   r12, r14, r12, asr #16   \n" /* OL7.OR7 */
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
        "stmfd   sp!, { lr }              \n"
        "ldr     r3, [r0, %1]             \n" /* r3 = pcm->amplitude */
        "ldr     r0, [r0, %0]             \n" /* r1 = pcm->addr */
        "subs    r2, r2, #1               \n"
        "beq     2f                       \n" /* 1 frame? */
    "1:  ldr     r14, [r0], #4            \n" /* M0.M1 */
        "subs    r2, r2, #2               \n"
        "smulwb  r12, r3, r14             \n" /* A*SM0 */
        "smulwt  r14, r3, r14             \n" /* A*SM1 */
        "pkhbt   r12, r12, r12, asl #16   \n" /* A*SM0.A*SM0 */
        "pkhbt   r14, r14, r14, asl #16   \n" /* A*SM1.A*SM1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { pc }              \n" /* event count? return */
    "2:  ldrh    r14, [r0]                \n" /* SM0 */
        "smulwb  r12, r3, r14             \n" /* A*SM0 */
        "pkhbt   r12, r12, r12, asl #16   \n" /* A*SM0.A*SM0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { pc }              \n"
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
    "1:  ldr     r12, [r0], #4            \n" /* SL0.SR0 */
        "ldr     r14, [r0], #4            \n" /* SL1.SR1 */
        "subs    r2, r2, #2               \n"
        "smulwb  r4, r3, r12              \n" /* A*SL0 */
        "smulwt  r12, r3, r12             \n" /* A*SR0 */
        "smulwb  r5, r3, r14              \n" /* A*SL1 */
        "smulwt  r14, r3, r14             \n" /* A*SR1 */
        "pkhbt   r4, r4, r12, asl #16     \n" /* A*SL0.A*SR0 */
        "pkhbt   r5, r5, r14, asl #16     \n" /* A*SL1.A*SR1 */
        "stmia   r1!, { r4-r5 }           \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r5, pc }       \n" /* event count? return */
    "2:  ldr     r12, [r0]                \n" /* SL0.SR0 */
        "smulwb  r4, r3, r12              \n" /* A*SL0 */
        "smulwt  r12, r3, r12             \n" /* A*SR0 */
        "pkhbt   r4, r4, r12, asl #16     \n" /* A*SL0.A*SR0 */
        "str     r4, [r1]                 \n"
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
    "1:  ldr     r4, [r0], #4             \n" /* SM0.SM1 */
        "subs    r2, r2, #2               \n"
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "pkhbt   r3, r4, r4, asl #16      \n" /* SM0.SM0 */
        "pkhtb   r4, r4, r4, asr #16      \n" /* SM1.SM1 */
        "qadd16  r12, r3, r12             \n" /* SM0+DL0.SM0+DR0 */
        "qadd16  r14, r4, r14             \n" /* SM1+DL1.SM1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldrh    r4, [r0]                 \n" /* SM0... */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */
        "pkhbt   r3, r4, r4, asl #16      \n" /* SM0.SM0 */
        "qadd16  r12, r3, r12             \n" /* SM0+DL0.SM0+DR0 */
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
    "1:  ldr     r3, [r0], #4             \n" /* SL0.SR0 */
        "ldr     r4, [r0], #4             \n" /* SL1.SR1 */
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "subs    r2, r2, #2               \n"
        "qadd16  r12, r12, r3             \n" /* SL0+DL0.SR0+DR0 */
        "qadd16  r14, r14, r4             \n" /* SL1+DL1.SR1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldr     r3, [r0]                 \n" /* SL0.SR0 */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */        
        "qadd16  r12, r12, r3             \n" /* SL0+DL0.SR0+DR0 */
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
        "subs    r2, r2, #2               \n"
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "smulwb  r4, r3, r5               \n" /* A*SM0 */
        "smulwt  r5, r3, r5               \n" /* A*SM1 */
        "pkhbt   r4, r4, r4, asl #16      \n" /* A*SM0.A*SM0 */
        "pkhbt   r5, r5, r5, asl #16      \n" /* A*SM1.A*SM1 */
        "qadd16  r12, r4, r12             \n" /* A*SM0+DL0.A*SM0+DR0 */
        "qadd16  r14, r5, r14             \n" /* A*SM1+DL1.A*SM1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r5, pc }       \n" /* event count? return */
    "2:  ldrh    r5, [r0]                 \n" /* SM0 */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */
        "smulwb  r4, r3, r5               \n" /* A*SM0 */
        "pkhbt   r4, r4, r4, asl #16      \n" /* A*SM0.A*SM0 */
        "qadd16  r12, r4, r12             \n" /* A*SM0+DL0.A*SM0+DR0 */
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
    "1:  ldr     r4, [r0], #4             \n" /* SL0.SR0 */
        "ldr     r6, [r0], #4             \n" /* SL1.SR1 */
        "subs    r2, r2, #2               \n"
        "ldmia   r1, { r12, r14 }         \n" /* DL0.DR0|DL1.DR1 */
        "smulwb  r5, r3, r4               \n" /* A*SL0 */
        "smulwt  r4, r3, r4               \n" /* A*SR0 */
        "smulwb  r7, r3, r6               \n" /* A*SL1 */
        "smulwt  r6, r3, r6               \n" /* A*SR1 */
        "pkhbt   r4, r5, r4, asl #16      \n" /* A*SL0.A*SR0 */
        "pkhbt   r6, r7, r6, asl #16      \n" /* A*SL1.A*SR1 */
        "qadd16  r12, r4, r12             \n" /* A*SL0+DR0.A*SR0+DR0 */
        "qadd16  r14, r6, r14             \n" /* A*SL1+DR1.A*SR1+DR1 */
        "stmia   r1!, { r12, r14 }        \n"
        "bhi     1b                       \n"
        "ldmlofd sp!, { r4-r7, pc }       \n" /* even count? return */
    "2:  ldr     r4, [r0]                 \n" /* SL0.R0 */
        "ldr     r12, [r1]                \n" /* DL0.DR0 */
        "smulwb  r5, r3, r4               \n" /* A*SL0 */
        "smulwt  r4, r3, r4               \n" /* A*SR0 */
        "pkhbt   r4, r5, r4, asl #16      \n" /* A*SL0.A*SR0 */
        "qadd16  r12, r4, r12             \n" /* A*SL0+DL0.A*SR0+DR0 */
        "str     r12, [r1]                \n"
        "ldmfd   sp!, { r4-r7, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_OFFS), "i"(PCM_STR_DESC_AMP_OFFS));

    (void)out; (void)desc; (void)count;
}
