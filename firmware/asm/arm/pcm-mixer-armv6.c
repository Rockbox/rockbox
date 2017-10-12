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

#ifndef write_samples_unity_1ch_s16i_s16i
#define write_samples_unity_1ch_s16i_s16i write_samples_unity_1ch_s16i_s16i_arm6

static void __attribute__((naked))
write_samples_unity_1ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                       void *out,
                                       unsigned long count)
{
    asm volatile (
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "ands     r3, r2, #0x7             \n"
        "beq      2f                       \n"
    "1:  ldrh     r12, [r0], #2            \n" /* first 1 to 7 frames */
        "subs     r3, r3, #1               \n" /* SM0 */
        "pkhbt    r12, r12, r12, asl #16   \n" /* OM0.OM0 */
        "str      r12, [r1], #4            \n"
        "bne      1b                       \n"
        "bics     r2, r2, #0x7             \n"
        "bxeq     lr                       \n"
    "2:  tst      r0, #2                   \n" /* word-aligned? */
        "stmeqfd  sp!, { r4-r8, lr }       \n"
        "stmnefd  sp!, { r4-r9, lr }       \n"
        "bne      4f                       \n"
    "3:  ldmia    r0!, { r3, r5, r7, r12 } \n" /* 8-chunk, word-aligned */
        "subs     r2, r2, #8               \n" /* SM1.SM0|SM3.SM2|SM5.SM4|SM7.SM6 */
        "pkhtb    r4, r3, r3, asr #16      \n" /* OM1.OM1 */
        "pkhbt    r3, r3, r3, asl #16      \n" /* OM0.OM0 */
        "pkhtb    r6, r5, r5, asr #16      \n" /* OM3.OM3 */
        "pkhbt    r5, r5, r5, asl #16      \n" /* OM2.OM2 */
        "pkhtb    r8, r7, r7, asr #16      \n" /* OM5.OM5 */
        "pkhbt    r7, r7, r7, asl #16      \n" /* OM4.OM4 */
        "pkhtb    r14, r12, r12, asr #16   \n" /* OM7.OM7 */
        "pkhbt    r12, r12, r12, asl #16   \n" /* OM6.OM6 */
        "stmia    r1!, { r3-r8, r12, r14 } \n"
        "bhi      3b                       \n"
        "ldmfd    sp!, { r4-r8, pc }       \n"
    "4:  ldrh     r3, [r0], #2             \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia    r0!, { r5, r7, r9, r14 } \n" /* SM0...|SM2.SM1|SM4.SM3|SM6.SM5|SM8?.SM7 */
        "subs     r2, r2, #8               \n"
        "pkhbt    r4, r3, r3, asl #16      \n" /* OM0.OM0 */
        "pkhtb    r6, r5, r5, asr #16      \n" /* OM2.OM2 */
        "pkhbt    r5, r5, r5, asl #16      \n" /* OM1.OM1 */
        "pkhtb    r8, r7, r7, asr #16      \n" /* OM4.OM4 */
        "pkhbt    r7, r7, r7, asl #16      \n" /* OM3.OM3 */
        "pkhtb    r12, r9, r9, asr #16     \n" /* OM6.OM6 */
        "pkhbt    r9, r9, r9, asl #16      \n" /* OM5.OM5 */
        "movhi    r3, r14, lsr #16         \n" /* SM8?... => SM0 for next loop */
        "pkhbt    r14, r14, r14, asl #16   \n" /* OM7.OM7 */
        "stmia    r1!, { r4-r9, r12, r14 } \n"
        "bhi      5b                       \n"
        "ldmfd    sp!, { r4-r9, pc }       \n"
    :
    : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_unity_1ch_s16i_s16i */

#ifndef write_samples_unity_2ch_s16i_s16i
#define write_samples_unity_2ch_s16i_s16i write_samples_unity_2ch_s16i_s16i_arm6

static void __attribute__((naked))
write_samples_unity_2ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                       void *out,
                                       unsigned long count)
{
    asm volatile (
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "ands     r3, r2, #0x7             \n"
        "beq      2f                       \n"
    "1:  ldr      r12, [r0], #4            \n" /* first 1 to 7 frames */
        "subs     r3, r3, #1               \n"
        "str      r12, [r1], #4            \n"
        "bne      1b                       \n"
        "bics     r2, r2, #0x7             \n"
        "bxeq     lr                       \n"
    "2:  tst      r0, #2                   \n" /* word aligned? */
        "stmeqfd  sp!, { r4-r8, lr }       \n"
        "stmnefd  sp!, { r4-r9, lr }       \n"
        "ldrneh   r3, [r0], #2             \n"
        "bne      5f                       \n"
    "3:  ldmia    r0!, { r3-r8, r12, r14 } \n" /* 8-chunk, word-aligned */
        "subs     r2, r2, #8               \n" /* SR0.SL0|SR1.SL1|SR2.SL2|SR3.SL3| ... */
        "stmia    r1!, { r3-r8, r12, r14 } \n" /* ... |SR4.SL4|SR5.SL5|SR6.SL6|SR7.SL7 */
        "bhi      3b                       \n"
        "ldmfd    sp!, { r4-r8, pc }       \n"
    "4:  mov      r3, r14                  \n" /* 8-chunk, halfword-aligned */
    "5:  ldmia    r0!, { r4-r9, r12, r14 } \n" /* SL0...|SL1.SR0|SL2.SR1|SL3.SR3|SL4.SR3| ... */
        "subs     r2, r2, #8               \n" /* ... |SL5.SR4|SL6.SR5|SL7.SR6|SL8?.SR7| */
        "pkhbt    r3, r3, r4, asl #16      \n" /* OR0.OL0 */
        "mov      r5, r5, ror #16          \n"
        "pkhtb    r4, r5, r4, asr #16      \n" /* OR1.OL1 */
        "pkhbt    r5, r5, r6, asl #16      \n" /* OR2.OL2 */
        "mov      r7, r7, ror #16          \n"
        "pkhtb    r6, r7, r6, asr #16      \n" /* OR3.OL3 */
        "pkhbt    r7, r7, r8, asl #16      \n" /* OR4.OL4 */
        "mov      r9, r9, ror #16          \n"
        "pkhtb    r8, r9, r8, asr #16      \n" /* OR5.OL5 */
        "pkhbt    r9, r9, r12, asl #16     \n" /* OR6.OL6 */
        "mov      r14, r14, ror #16        \n"
        "pkhtb    r12, r14, r12, asr #16   \n" /* OR7.OL7 */
        "stmia    r1!, { r3-r9, r12 }      \n"
        "bhi      4b                       \n"
        "ldmfd    sp!, { r4-r9, pc }       \n"
    :
    : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_unity_2ch_s16i_s16i */

#ifndef write_samples_gain_1ch_s16i_s16i
#define write_samples_gain_1ch_s16i_s16i  write_samples_gain_1ch_s16i_s16i_arm6

static void __attribute__((naked))
write_samples_gain_1ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                      void *out,
                                      unsigned long count)
{
    asm volatile(
        "ldr      r3, [r0, %1]             \n" /* r3 = desc->amplitude_tx */
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "tst      r0, #2                   \n" /* word/halfword aligned? */
        "bne      3f                       \n"
        "stmfd    sp!, { r4, lr }          \n"
        "subs     r2, r2, #1               \n"
        "beq      2f                       \n" /* 1 frame? */
    "1:  ldr      r4, [r0], #4             \n" /* M0.M1 */
        "subs     r2, r2, #2               \n"
        "smulwb   r12, r3, r4              \n" /* A*SM0 */
        "smulwt   r14, r3, r4              \n" /* A*SM1 */
        "pkhbt    r12, r12, r12, asl #16   \n" /* A*SM0.A*SM0 */
        "pkhbt    r14, r14, r14, asl #16   \n" /* A*SM1.A*SM1 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      1b                       \n"
        "ldmlofd  sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldrh     r14, [r0]                \n" /* SM0 */
        "smulwb   r12, r3, r14             \n" /* A*SM0 */
        "pkhbt    r12, r12, r12, asl #16   \n" /* A*SM0.A*SM0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4, pc }          \n"
    "3:  stmfd    sp!, { r4-r5, lr }       \n"
        "ldrh     r12, [r0], #2            \n" /* ....SM0 */
        "subs     r2, r2, #1               \n" /* 1 frame? */
        "smulwb   r4, r3, r12              \n" /* A*SM0 */
        "beq      5f                       \n"
    "4:  ldr      r5, [r0], #4             \n" /* SM2.SM1 */
        "subs     r2, r2, #2               \n"
        "pkhbt    r12, r4, r4, asl #16     \n" /* A*SM0.A*SM0 */
        "smulwb   r14, r3, r5              \n" /* A*SM1 */
        "smulwths r4, r3, r5               \n" /* A*SM2 => A*SM0 */
        "pkhbt    r14, r14, r14, asl #16   \n" /* A*SM1.A*SM1 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      4b                       \n"
        "ldmlofd  sp!, { r4-r5, pc }       \n" /* event count? return */     
    "5:  pkhbt    r12, r4, r4, asl #16     \n" /* A*SM0.A*SM0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r5, pc }       \n"
        
        
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_gain_1ch_s16i_s16i */

#ifndef write_samples_gain_2ch_s16i_s16i
#define write_samples_gain_2ch_s16i_s16i  write_samples_gain_2ch_s16i_s16i_arm6

static void __attribute__((naked))
write_samples_gain_2ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                      void *out,
                                      unsigned long count)
{
    asm volatile(
        "ldr      r3, [r0, %1]             \n" /* r3 = desc->amplitude_tx */
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "tst      r0, #2                   \n" /* word/halfword aligned? */
        "bne      3f                       \n"
        "stmfd    sp!, { r4-r5, lr }       \n"
        "subs     r2, r2, #1               \n"
        "beq      2f                       \n" /* 1 frame? */
    "1:  ldmia    r0!, { r12, r14 }        \n" /* SR0.SL0|SR1.SL1 */
        "subs     r2, r2, #2               \n"
        "smulwb   r4, r3, r12              \n" /* A*SL0 */
        "smulwt   r12, r3, r12             \n" /* A*SR0 */
        "smulwb   r5, r3, r14              \n" /* A*SL1 */
        "smulwt   r14, r3, r14             \n" /* A*SR1 */
        "pkhbt    r4, r4, r12, asl #16     \n" /* A*SR0.A*SL0 */
        "pkhbt    r5, r5, r14, asl #16     \n" /* A*SR1.A*SL1 */
        "stmia    r1!, { r4-r5 }           \n"
        "bhi      1b                       \n"
        "ldmlofd  sp!, { r4-r5, pc }       \n" /* event count? return */
    "2:  ldr      r12, [r0]                \n" /* SR0.SL0 */
        "smulwb   r4, r3, r12              \n" /* A*SL0 */
        "smulwt   r12, r3, r12             \n" /* A*SR0 */
        "pkhbt    r4, r4, r12, asl #16     \n" /* A*SL0.A*SR0 */
        "str      r4, [r1]                 \n"
        "ldmfd    sp!, { r4-r5, pc }       \n"
    "3:  stmfd    sp!, { r4-r6, lr }       \n"
        "ldrh     r12, [r0], #2            \n" /* ....SL0 */
        "subs     r2, r2, #1               \n"
        "smulwb   r4, r3, r12              \n" /* A*SL0 */
        "beq      5f                       \n"
    "4:  ldmia    r0!, { r12, r14 }        \n" /* SL1.SR0|SL2.SR1 */
        "subs     r2, r2, #2               \n"
        "smulwb   r5, r3, r12              \n" /* A*SR0 */
        "smulwt   r6, r3, r12              \n" /* A*SL1 */
        "smulwb   r12, r3, r14             \n" /* A*SR1 */
        "pkhbt    r5, r4, r5, asl #16      \n" /* A*SR0.A*SL0 */
        "smulwths r4, r3, r14              \n" /* A*SL2 => A*SL0 */
        "pkhbt    r6, r6, r12, asl #16     \n" /* A*SR1.A*SL1 */
        "stmia    r1!, { r5-r6 }           \n"
        "bhi      4b                       \n"
        "ldmlofd  sp!, { r4-r6, pc }       \n"
    "5:  ldr      r12, [r0]                \n" /* SL1.SR0 */
        "smulwb   r5, r3, r12              \n" /* A*SR0 */
        "pkhbt    r5, r4, r5, asl #16      \n" /* A*SR0.A*SL0 */
        "str      r5, [r1]                 \n"
        "ldmfd    sp!, { r4-r6, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* write_samples_gain_2ch_s16i_s16i */

#ifndef mix_samples_unity_1ch_s16i_s16i
#define mix_samples_unity_1ch_s16i_s16i   mix_samples_unity_1ch_s16i_s16i_arm6

static void __attribute__((naked))
mix_samples_unity_1ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                     void *out,
                                     unsigned long count)
{
    asm volatile (
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "tst      r0, #2                   \n" /* word/halfword aligned? */
        "bne      3f                       \n"
        "stmfd    sp!, { r4, lr }          \n"
        "subs     r2, r2, #1               \n" /* 1 frame? */
        "beq      2f                       \n"
    "1:  ldr      r4, [r0], #4             \n" /* SM1.SM0 */
        "subs     r2, r2, #2               \n"
        "ldm      r1, { r12, r14 }         \n" /* DR0.DL0|DR1.DL1 */
        "pkhbt    r3, r4, r4, asl #16      \n" /* SM0.SM0 */
        "pkhtb    r4, r4, r4, asr #16      \n" /* SM1.SM1 */
        "qadd16   r12, r3, r12             \n" /* SM0+DR0.SM0+DL0 */
        "qadd16   r14, r4, r14             \n" /* SM1+DR1.SM1+DL1 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      1b                       \n"
        "ldmlofd  sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldrh     r4, [r0]                 \n" /* ...SM0 */
        "ldr      r12, [r1]                \n" /* DR0.DL0 */
        "pkhbt    r3, r4, r4, asl #16      \n" /* SM0.SM0 */
        "qadd16   r12, r3, r12             \n" /* SM0+DR0.SM0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4, pc }          \n"
    "3:  stmfd    sp!, { r4-r5, lr }       \n"
        "ldrh     r3, [r0], #2             \n" /* ....SM0 */
        "subs     r2, r2, #1               \n"
        "pkhbt    r3, r3, r3, asl #16      \n" /* SM0.SM0 */
        "beq      5f                       \n" /* 1 frame? */
    "4:  ldr      r5, [r0], #4             \n" /* SM2.SM1 */
        "ldm      r1, { r12, r14 }         \n"
        "subs     r2, r2, #2               \n"
        "pkhbt    r4, r5, r5, asl #16      \n" /* SM1.SM1 */
        "qadd16   r12, r3, r12             \n" /* SM0+DR0.SM0+DL0 */
        "qadd16   r14, r4, r14             \n" /* SM1+DR1.SM1+DL1 */
        "pkhtbhs  r3, r5, r5, asr #16      \n" /* SM2=>SM0.SM0 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      4b                       \n"
        "ldmlofd  sp!, { r4-r5, pc }       \n"
    "5:  ldr      r12, [r1]                \n" /* DR0.DL0 */
        "qadd16   r12, r3, r12             \n" /* SM0+DR0.SM0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r5, pc }       \n"  
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_unity_1ch_s16i_s16i */

#ifndef mix_samples_unity_2ch_s16i_s16i
#define mix_samples_unity_2ch_s16i_s16i   mix_samples_unity_2ch_s16i_s16i_arm6

static void __attribute__((naked))
mix_samples_unity_2ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                     void *out,
                                     unsigned long count)
{
    asm volatile (
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "tst      r0, #2                   \n" /* word/halfword aligned? */
        "bne      3f                       \n"
        "stmfd    sp!, { r4, lr }          \n"
        "subs     r2, r2, #1               \n" /* 1 frame? */
        "beq      2f                       \n"
    "1:  ldmia    r0!, { r3-r4 }           \n" /* SR0.SL0|SR1.SL1 */
        "ldm      r1, { r12, r14 }         \n" /* DR0.DL0|DR1.DL1 */
        "subs     r2, r2, #2               \n"
        "qadd16   r12, r12, r3             \n" /* SR0+DR0.SL0+DL0 */
        "qadd16   r14, r14, r4             \n" /* SR1+DR1.SL1+DL1 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      1b                       \n"
        "ldmlofd  sp!, { r4, pc }          \n" /* event count? return */
    "2:  ldr      r3, [r0]                 \n" /* SR0.SL0 */
        "ldr      r12, [r1]                \n" /* DR0.DL0 */
        "qadd16   r12, r12, r3             \n" /* SR0+DR0.SL0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4, pc }          \n"
    "3:  stmfd    sp!, { r4-r5, pc }       \n"
        "ldrh     r3, [r0], #2             \n" /* ....SL0 */
        "subs     r2, r2, #1               \n" /* 1 frame? */
        "beq      5f                       \n"
    "4:  ldmia    r0!, { r4-r5 }           \n" /* SL1.SR0|SL2.SR1 */
        "ldm      r1, { r12, r14 }         \n" /* DR0.DL0|DR1.DL1 */
        "pkhbt    r3, r3, r4, asl #16      \n" /* SR0.SL0 */
        "mov      r4, r4, lsr #16          \n" /* SR1.SL1 */
        "orr      r4, r5, lsl #16          \n"
        "qadd16   r12, r12, r3             \n" /* SR0+DR0.SL0+DL0 */
        "qadd16   r14, r14, r4             \n" /* SR1+DR1.SL1+DL1 */
        "movhs    r3, r5, lsr #16          \n" /* SL2 => SL0 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      4b                       \n"
        "ldmlofd  sp!, { r4-r5, pc }       \n"
    "5:  ldr      r4, [r0]                 \n" /* SL1.SR0 */
        "ldr      r12, [r1]                \n" /* DR0.DL0 */
        "pkhbt    r3, r3, r4, asl #16      \n" /* SR0,SL0 */
        "qadd16   r12, r12, r3             \n" /* SR0+DR0.SL0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r5, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_unity_2ch_s16i_s16i */

#ifndef mix_samples_gain_1ch_s16i_s16i
#define mix_samples_gain_1ch_s16i_s16i    mix_samples_gain_1ch_s16i_s16i_arm6

static void __attribute__((naked))
mix_samples_gain_1ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                    void *out,
                                    unsigned long count)
{
    asm volatile (
        "ldr      r3, [r0, %1]             \n" /* r3 = desc->amplitude_tx */
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "tst      r0, #2                   \n" /* word/halfword aligned? */
        "bne      3f                       \n"
        "stmfd    sp!, { r4-r5, lr }       \n"
        "subs     r2, r2, #1               \n"
        "beq      2f                       \n" /* 1 frame? */
    "1:  ldr      r5, [r0], #4             \n" /* M1.M0 */
        "subs     r2, r2, #2               \n"
        "ldm      r1, { r12, r14 }         \n" /* DR0.DL0|DR1.DL1 */
        "smulwb   r4, r3, r5               \n" /* A*SM0 */
        "smulwt   r5, r3, r5               \n" /* A*SM1 */
        "pkhbt    r4, r4, r4, asl #16      \n" /* A*SM0.A*SM0 */
        "pkhbt    r5, r5, r5, asl #16      \n" /* A*SM1.A*SM1 */
        "qadd16   r12, r4, r12             \n" /* A*SM0+DR0.A*SM0+DL0 */
        "qadd16   r14, r5, r14             \n" /* A*SM1+DR1.A*SM1+DL1 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      1b                       \n"
        "ldmlofd  sp!, { r4-r5, pc }       \n" /* event count? return */
    "2:  ldrh     r5, [r0]                 \n" /* SM0 */
        "ldr      r12, [r1]                \n" /* DR0.DL0 */
        "smulwb   r4, r3, r5               \n" /* A*SM0 */
        "pkhbt    r4, r4, r4, asl #16      \n" /* A*SM0.A*SM0 */
        "qadd16   r12, r4, r12             \n" /* A*SM0+DR0.A*SM0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r5, pc }       \n"
    "3:  stmfd    sp!, { r4-r6, lr }       \n"
        "ldrh     r12, [r0], #2            \n" /* ....SM0 */
        "subs     r2, r2, #1               \n" /* 1 frame? */
        "smulwb   r4, r3, r12              \n" /* A*SM0 */
        "beq      5f                       \n"
    "4:  ldr      r6, [r0], #4             \n" /* SM2.SM1 */
        "ldm      r1, { r12, r14 }         \n" /* DR0.DL0|DR1.DL1 */
        "subs     r2, r2, #2               \n"
        "smulwb   r5, r3, r6               \n" /* A*SM1 */
        "pkhbt    r4, r4, r4, asl #16      \n" /* A*SM0.A*SM0 */
        "pkhbt    r5, r5, r5, asl #16      \n" /* A*SM1.A*SM1 */
        "qadd16   r12, r4, r12             \n" /* A*SM0+DR0.A*SM0+DL0 */
        "qadd16   r14, r5, r14             \n" /* A*SM1+DR1.A*SM1+DL1 */
        "smulwths r4, r3, r6               \n" /* A*SM2 => A*SM0 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      4b                       \n"
        "ldmlofd  sp!, { r4-r6, pc }       \n"
    "5:  ldr      r12, [r1]                \n" /* DR0.DL0 */
        "pkhbt    r4, r4, r4, asl #16      \n" /* A*SM0.A*SM0 */
        "qadd16   r12, r4, r12             \n" /* A*SM0+DR0.A*SM0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r6, pc }       \n"
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_gain_1ch_s16i_s16i */

#ifndef mix_samples_gain_2ch_s16i_s16i
#define mix_samples_gain_2ch_s16i_s16i    mix_samples_gain_2ch_s16i_s16i_arm6

static void __attribute__((naked))
mix_samples_gain_2ch_s16i_s16i_arm6(struct pcm_stream_desc *desc,
                                    void *out,
                                    unsigned long count)
{
    asm volatile (
        "ldr      r3, [r0, %1]             \n" /* r3 = desc->amplitude_tx */
        "ldr      r0, [r0, %0]             \n" /* r0 = desc->addr_tx */
        "tst      r0, #2                   \n" /* word/halfword aligned? */
        "bne      3f                       \n"
        "stmfd    sp!, { r4-r7, lr }       \n"
        "subs     r2, r2, #1               \n"
        "beq      2f                       \n" /* 1 frame? */
    "1:  ldmia    r0!, { r4, r6 }          \n" /* SR0.SL0|SR1.SL1 */
        "subs     r2, r2, #2               \n"
        "ldm      r1, { r12, r14 }         \n" /* DR0.DL0|DR1.DL1 */
        "smulwb   r5, r3, r4               \n" /* A*SL0 */
        "smulwt   r4, r3, r4               \n" /* A*SR0 */
        "smulwb   r7, r3, r6               \n" /* A*SL1 */
        "smulwt   r6, r3, r6               \n" /* A*SR1 */
        "pkhbt    r4, r5, r4, asl #16      \n" /* A*SR0.A*SL0 */
        "pkhbt    r6, r7, r6, asl #16      \n" /* A*SR1.A*SL1 */
        "qadd16   r12, r4, r12             \n" /* A*SR0+DR0.A*SL0+DL0 */
        "qadd16   r14, r6, r14             \n" /* A*SR1+DR1.A*SL1+DL1 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      1b                       \n"
        "ldmlofd  sp!, { r4-r7, pc }       \n" /* even count? return */
    "2:  ldr      r4, [r0]                 \n" /* SR0.SL0 */
        "ldr      r12, [r1]                \n" /* DR0.DL0 */
        "smulwb   r5, r3, r4               \n" /* A*SL0 */
        "smulwt   r4, r3, r4               \n" /* A*SR0 */
        "pkhbt    r4, r5, r4, asl #16      \n" /* A*SR0.A*SL0 */
        "qadd16   r12, r4, r12             \n" /* A*SR0+DR0.A*SL0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r7, pc }       \n"
    "3:  stmfd    sp!, { r4-r8, lr }       \n"        
        "ldrh     r12, [r0], #2            \n" /* ....SL0 */
        "subs     r2, r2, #1               \n" /* 1 frame? */
        "smulwb   r4, r3, r12              \n" /* A*SL0 */
        "beq      5f                       \n"
    "4:  ldmia    r0!, { r5, r8 }          \n" /* SL1.SR0|SL2.SR1 */
        "subs     r2, r2, #2               \n"
        "ldm      r1, { r12, r14 }         \n"
        "smulwb   r6, r3, r5               \n" /* A*SR0 */
        "smulwt   r5, r3, r5               \n" /* A*SL1 */
        "smulwb   r7, r3, r8               \n" /* A*SR1 */
        "pkhbt    r4, r4, r6, asl #16      \n" /* A*SR0.A*SL0 */
        "pkhbt    r5, r5, r7, asl #16      \n" /* A*SR1.A*SL1 */
        "qadd16   r12, r4, r12             \n" /* A*SR0+DR0.A*SL0+DL0 */
        "qadd16   r14, r5, r14             \n" /* A*SR1+DR1.A*SL1+DL1 */
        "smulwths r4, r3, r8               \n" /* A*SL2 => A*SL0 */
        "stmia    r1!, { r12, r14 }        \n"
        "bhi      4b                       \n"
        "ldmlofd  sp!, { r4-r8, pc }       \n" /* even count? return */
    "5:  ldr      r5, [r0]                 \n" /* SL1.SR0 */
        "ldr      r12, [r1]                \n" /* DR0.DL1 */
        "smulwb   r6, r3, r5               \n" /* A*SR0 */
        "pkhbt    r4, r4, r6, asl #16      \n" /* A*SR0.A*SL0 */
        "qadd16   r12, r4, r12             \n" /* A*SR0+DR0.A*SL0+DL0 */
        "str      r12, [r1]                \n"
        "ldmfd    sp!, { r4-r8, pc }       \n" /* even count? return */
        :
        : "i"(PCM_STR_DESC_ADDR_TX_OFFS), "i"(PCM_STR_DESC_AMP_TX_OFFS));

    (void)out; (void)desc; (void)count;
}
#endif /* mix_samples_gain_2ch_s16i_s16i */
