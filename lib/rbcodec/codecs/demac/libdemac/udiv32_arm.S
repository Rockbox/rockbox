/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jens Arnold
 * Copyright (C) 2009 by Andrew Mahone
 *
 * Optimised unsigned integer division for ARMv4
 *
 * Based on: libgcc routines for ARM cpu, additional algorithms from ARM System
 *           Developer's Guide
 * Division routines, written by Richard Earnshaw, (rearnsha@armltd.co.uk)
 * Copyright 1995, 1996, 1998, 1999, 2000, 2003, 2004, 2005
 * Free Software Foundation, Inc.
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

#include "config.h"
/* On targets with codec iram, a header file will be generated after an initial
   link of the APE codec, stating the amount of IRAM remaining for use by the
   reciprocal lookup table. */
#if !defined(APE_PRE) && defined(USE_IRAM) && ARM_ARCH < 5
#include "lib/rbcodec/codecs/ape_free_iram.h"
#endif

/* Codecs should not normally do this, but we need to check a macro, and
 * codecs.h would confuse the assembler. */

#ifdef USE_IRAM
#define DIV_RECIP
    .section    .icode,"ax",%progbits
#else
    .text
#endif
    .align
    .global udiv32_arm
    .type   udiv32_arm,%function

#if ARM_ARCH < 5
/* Adapted from an algorithm given in ARM System Developer's Guide (7.3.1.2)
   for dividing a 30-bit value by a 15-bit value, with two operations per
   iteration by storing quotient and remainder together and adding the previous
   quotient bit during trial subtraction. Modified to work with any dividend
   and divisor both less than 1 << 30, and skipping trials by calculating bits
   in output. */
.macro ARM_DIV_31_BODY dividend, divisor, result, bits, curbit, quotient, remainder

    mov     \bits, #1
    /* Shift the divisor left until it aligns with the numerator. If it already
       has the high bit set, this is fine, everything inside .rept will be
       skipped, and the add before and adcs after will set the one-bit result
       to zero. */
    cmn     \divisor, \dividend, lsr #16
    movcs   \divisor, \divisor, lsl #16
    addcs   \bits, \bits, #16
    cmn     \divisor, \dividend, lsr #8
    movcs   \divisor, \divisor, lsl #8
    addcs   \bits, \bits, #8
    cmn     \divisor, \dividend, lsr #4
    movcs   \divisor, \divisor, lsl #4
    addcs   \bits, \bits, #4
    cmn     \divisor, \dividend, lsr #2
    movcs   \divisor, \divisor, lsl #2
    addcs   \bits, \bits, #2
    cmn     \divisor, \dividend, lsr #1
    movcs   \divisor, \divisor, lsl #1
    addcs   \bits, \bits, #1
    adds    \result, \dividend, \divisor
    subcc   \result, \result, \divisor
    rsb     \curbit, \bits, #31
    add     pc, pc, \curbit, lsl #3
    nop
    .rept   30
    adcs    \result, \divisor, \result, lsl #1
    /* Fix the remainder portion of the result. This must be done because the
       handler for 32-bit numerators needs the remainder. */
    subcc   \result, \result, \divisor
    .endr
    /* Shift remainder/quotient left one, add final quotient bit */
    adc     \result, \result, \result
    mov     \remainder, \result, lsr \bits
    eor     \quotient, \result, \remainder, lsl \bits
.endm

#ifndef FREE_IRAM
.set recip_max, 2
#else
/* Each table entry is one word. Since a compare is done against the maximum
   entry as an immediate, the maximum entry must be a valid ARM immediate,
   which means a byte shifted by an even number of places. */
.set recip_max, 2 + FREE_IRAM / 4
.set recip_max_tmp, recip_max >> 8
.set recip_mask_shift, 0
.set tmp_shift, 16
.rept 5
    .if recip_max_tmp >> tmp_shift
        .set recip_max_tmp, recip_max_tmp >> tmp_shift
        .set recip_mask_shift, recip_mask_shift + tmp_shift
    .endif
    .set tmp_shift, tmp_shift >> 1
.endr
.if recip_max_tmp
    .set recip_mask_shift, recip_mask_shift + 1
.endif
.set recip_mask_shift, (recip_mask_shift + 1) & 62
.set recip_max, recip_max & (255 << recip_mask_shift)
//.set recip_max, 2
#endif

udiv32_arm:
#ifdef DIV_RECIP
    cmp     r1, #3
    bcc     .L_udiv_tiny
    cmp     r1, #recip_max
    bhi     .L_udiv
    adr     r3, .L_udiv_recip_table-12
    ldr     r2, [r3, r1, lsl #2]
    mov     r3, r0
    umull   ip, r0, r2, r0
    mul     r2, r0, r1
    cmp     r3, r2
    bxcs    lr
    sub     r0, r0, #1
    bx      lr
.L_udiv_tiny:
    cmp     r1, #1
    movhi   r0, r0, lsr #1
    bxcs    lr
    b       .L_div0
#endif
.L_udiv:
    /* Invert divisor. ARM_DIV_31_BODY uses adc to both subtract the divisor
       and add the next bit of the result. The correction code at .L_udiv32
       does not need the divisor inverted, but can be modified to work with it,
       and this allows the zero divisor test to be done early and without an
       explicit comparison. */
    rsbs    r1, r1, #0
#ifndef DIV_RECIP
    beq .L_div0
#endif
    tst     r0, r0
    /* High bit must be unset, otherwise shift numerator right, calculate,
       and correct results. As this case is very uncommon we want to avoid
       any other delays on the main path in handling it, so the long divide
       calls the short divide as a function. */
    bmi     .L_udiv32
.L_udiv31:
    ARM_DIV_31_BODY r0, r1, r2, r3, ip, r0, r1
    bx      lr
.L_udiv32:
    /* store original numerator and divisor, we'll need them to correct the
       result, */
    stmdb   sp, { r0, r1, lr }
    /* Call __div0 here if divisor is zero, otherwise it would report the wrong
       address. */
    mov     r0, r0, lsr #1
    bl      .L_udiv31
    ldmdb   sp, { r2, r3, lr }
    /* Move the low bit of the original numerator to the carry bit */
    movs    r2, r2, lsr #1
    /* Shift the remainder left one and add in the carry bit */
    adc     r1, r1, r1
    /* Subtract the original divisor from the remainder, setting carry if the
       result is non-negative */
    adds    r1, r1, r3
    /* Shift quotient left one and add carry bit */
    adc     r0, r0, r0
    bx      lr
.L_div0:
    /* __div0 expects the calling address on the top of the stack */
    stmdb sp!, { lr }
    mov     r0, #0
#if defined(__ARM_EABI__) || !defined(USE_IRAM)
    bl      __div0
#else
    ldr     pc, [pc, #-4]
    .word   __div0
#endif
#ifdef DIV_RECIP
.L_udiv_recip_table:
    .set div, 3
    .rept recip_max - 2
        .if (div - 1) & div
            .set q, 0x40000000 / div
            .set r, (0x40000000 - (q * div))<<1
            .set q, q << 1
            .if r >= div
                .set q, q + 1
                .set r, r - div
            .endif
            .set r, r << 1
            .set q, q << 1
            .if r >= div
                .set q, q + 1
                .set r, r - div
            .endif
            .set q, q + 1
        .else
            .set q, 0x40000000 / div * 4
        .endif
        .word q
        .set div, div+1
    .endr
#endif
    .size udiv32_arm, . - udiv32_arm

#else
.macro ARMV5_UDIV32_BODY numerator, divisor, quotient, bits, inv, neg, div0label
    cmp     \numerator, \divisor
    clz     \bits, \divisor
    bcc     30f
    mov     \inv, \divisor, lsl \bits
    add     \neg, pc, \inv, lsr #25
    cmp     \inv, #1<<31
    ldrhib  \inv, [\neg, #.L_udiv_est_table-.-64]
    bls     20f
    subs    \bits, \bits, #7
    rsb     \neg, \divisor, #0
    movpl   \divisor, \inv, lsl \bits
    bmi     10f
    mul     \inv, \divisor, \neg
    smlawt  \divisor, \divisor, \inv, \divisor
    mul     \inv, \divisor, \neg
    /* This will save a cycle on ARMv6, but requires that the numerator sign
       bit is not set (that of inv is guaranteed unset). The branch should
       predict very well, making it typically 1 cycle, and thus both the branch
       and test fill delay cycles for the multiplies. Based on logging of
       numerator sizes in the APE codec, the branch is taken about 1/10^7 of
       the time. */
#if ARM_ARCH >= 6
    tst     \numerator, \numerator
    smmla   \divisor, \divisor, \inv, \divisor
    bmi     40f
    smmul   \inv, \numerator, \divisor
#else
    mov     \bits, #0
    smlal   \bits, \divisor, \inv, \divisor
    umull   \bits, \inv, \numerator, \divisor
#endif
    add     \numerator, \numerator, \neg
    mla     \divisor, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \divisor, \neg
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
    bx      lr
10:
    rsb     \bits, \bits, #0
    sub     \inv, \inv, #4
    mov     \divisor, \inv, lsr \bits
    umull   \bits, \inv, \numerator, \divisor
    mla     \divisor, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \neg, \divisor, lsr #1
    addcs   \divisor, \divisor, \neg, lsl #1
    addcs   \quotient, \quotient, #2
    cmn     \neg, \divisor
    addcs   \quotient, \quotient, #1
    bx      lr
20:
.ifnc "", "\div0label"
    rsb     \bits, \bits, #31
    bne     \div0label
.endif
    mov     \quotient, \numerator, lsr \bits
    bx      lr
30:
    mov     \quotient, #0
    bx      lr
#if ARM_ARCH >= 6
40:
    umull   \bits, \inv, \numerator, \divisor
    add     \numerator, \numerator, \neg
    mla     \divisor, \inv, \neg, \numerator
    mov     \quotient, \inv
    cmn     \divisor, \neg
    addcc   \quotient, \quotient, #1
    addpl   \quotient, \quotient, #2
    bx      lr
#endif
.endm

udiv32_arm:
    ARMV5_UDIV32_BODY r0, r1, r0, r2, r3, ip, .L_div0
.L_div0:
    /* __div0 expects the calling address on the top of the stack */
    stmdb sp!, { lr }
    mov     r0, #0
#if defined(__ARM_EABI__) || !defined(USE_IRAM)
    bl      __div0
#else
    ldr     pc, [pc, #-4]
    .word   __div0
#endif
.L_udiv_est_table:
    .byte 0xff, 0xfc, 0xf8, 0xf4, 0xf0, 0xed, 0xea, 0xe6
    .byte 0xe3, 0xe0, 0xdd, 0xda, 0xd7, 0xd4, 0xd2, 0xcf
    .byte 0xcc, 0xca, 0xc7, 0xc5, 0xc3, 0xc0, 0xbe, 0xbc
    .byte 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0, 0xae, 0xac
    .byte 0xaa, 0xa8, 0xa7, 0xa5, 0xa3, 0xa2, 0xa0, 0x9f
    .byte 0x9d, 0x9c, 0x9a, 0x99, 0x97, 0x96, 0x94, 0x93
    .byte 0x92, 0x90, 0x8f, 0x8e, 0x8d, 0x8c, 0x8a, 0x89
    .byte 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81
#endif
    .size udiv32_arm, . - udiv32_arm
