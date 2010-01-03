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
 * Based on: libgcc routines for ARM cpu.
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
/* Codecs should not normally do this, but we need to check a macro, and
 * codecs.h would confuse the assembler. */

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
    cmp     \divisor, \dividend, lsr #16
    movls   \divisor, \divisor, lsl #16
    addls   \bits, \bits, #16
    cmp     \divisor, \dividend, lsr #8
    movls   \divisor, \divisor, lsl #8
    addls   \bits, \bits, #8
    cmp     \divisor, \dividend, lsr #4
    movls   \divisor, \divisor, lsl #4
    addls   \bits, \bits, #4
    cmp     \divisor, \dividend, lsr #2
    movls   \divisor, \divisor, lsl #2
    addls   \bits, \bits, #2
    cmp     \divisor, \dividend, lsr #1
    movls   \divisor, \divisor, lsl #1
    addls   \bits, \bits, #1
    rsbs    \divisor, \divisor, #0
    bcs     .L_div0
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

#ifdef USE_IRAM
    .section    .icode,"ax",%progbits
#else
    .text
#endif
    .align
    .global udiv32_arm
    .type   udiv32_arm,%function

udiv32_arm:
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
    /* This address is never a branch target, but is used to test lr before
       calling __div0. */
.L_udiv32_div0_trap:
    ldmdb   sp, { r2, r3, lr }
    /* Move the low bit of the original numerator to the carry bit */
    movs    r2, r2, lsr #1
    /* Shift the remainder left one and add in the carry bit */
    adc     r1, r1, r1
    /* Subtract the original divisor from the remainder, setting carry if the
       result is non-negative */
    subs    r1, r1, r3
    /* Shift quotient left one and add carry bit */
    adc     r0, r0, r0
    bx      lr
.L_div0:
    /* Check the return address, since .L_udiv32 uses bl to wrap the 31-bit
       divider. If the return address is at .L_udiv32_div0_trap, then the 
       the return address of the original caller is at sp - 4
    */
    adr     r2, .L_udiv32_div0_trap
    cmp     r2, lr
    subeq     sp, sp, #4
    bleq    __div0
    /* Otherwise, push lr to the stack before calling __div0 */
    stmdb sp!, { lr }
    bl      __div0
    .size udiv32_arm, . - udiv32_arm
