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
   in output.
*/
.macro ARM_DIV_31_BODY dividend, divisor, result, bits, curbit, quotient

    mov     \bits, #1
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
    rsb     \divisor, \divisor, #0
    adds    \result, \dividend, \divisor
    subcc   \result, \result, \divisor
    rsb     \curbit, \bits, #31
    add     pc, pc, \curbit, lsl #3
    nop
    .rept   30
    adcs    \result, \divisor, \result, lsl #1
    subcc   \result, \result, \divisor
    .endr
    /* shift remainder/quotient left one, add final quotient bit */
    adc     \result, \result, \result
    mov     \dividend, \result, lsr \bits
    eor     \quotient, \result, \dividend, lsl \bits
.endm

.macro ARM_DIV_32_BODY dividend, divisor, result, curbit

    mov     \result, \dividend
    mov     \curbit, #90          @ 3 * 30, (calculating branch dest)
    cmp     \divisor, \result, lsr #16
    movls   \result,\result, lsr #16
    subls   \curbit, \curbit, #48
    cmp     \divisor, \result, lsr #8
    movls   \result,\result, lsr #8
    subls   \curbit, \curbit, #24
    cmp     \divisor, \result, lsr #4
    movls   \result,\result, lsr #4
    subls   \curbit, \curbit, #12
    cmp     \divisor, \result, lsr #2
    subls   \curbit, \curbit, #6
    @ Calculation is only done down to shift=2, because the shift=1 step
    @ would need 3 more cycles, but would only gain 1.5 cycles on average.
    mov     \result, #0
    add     pc, pc, \curbit, lsl #2
    nop
    .set    shift, 32
    .rept   31
    .set    shift, shift - 1
    cmp     \divisor, \dividend, lsr #shift
    orrls   \result, \result, #(1 << shift)
    subls   \dividend, \dividend, \divisor, lsl #shift
    .endr   @ shift==0 in the .rept would cause a warning  for lsr #0
    cmp     \divisor, \dividend
    orrls   \result, \result, #1
    @subls  \dividend, \dividend, \divisor  @ correct remainder not needed
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
    cmp     r1, #0
    beq     20f
    tst     r0, r0
    /* High bit must be unset, otherwise use ARM_DIV_32_BODY. High bit of
       divisor is also unset dividend has been tested to be >= divisor.
    */
    bmi     10f
    ARM_DIV_31_BODY r0, r1, r2, r3, ip, r0
    bx      lr

10:
    ARM_DIV_32_BODY r0, r1, r2, r3
    mov     r0, r2
    bx      lr

20:
    movne   r0, #0
    bx      lr
