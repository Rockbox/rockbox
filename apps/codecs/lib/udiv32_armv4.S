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

.macro ARM_DIV_BODY dividend, divisor, result, curbit

    mov     \result, \dividend
    mov     \curbit, #93          @ 3 * 31, (calculating branch dest)
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
    movls   \result,\result, lsr #2
    subls   \curbit, \curbit, #6
    cmp     \divisor, \result, lsr #1
    subls   \curbit, \curbit, #3
    mov     \result, #0
    add     pc, pc, \curbit, lsl #2
    nop
    .set    shift, 32
    .rept   32
    .set    shift, shift - 1
    cmp     \dividend, \divisor, lsl #shift
    adc     \result, \result, \result
    subcs   \dividend, \dividend, \divisor, lsl #shift
    .endr
.endm

.macro ARM_DIV2_ORDER divisor, order

    cmp     \divisor, #(1 << 16)
    movhs   \divisor, \divisor, lsr #16
    movhs   \order, #16
    movlo   \order, #0

    cmp     \divisor, #(1 << 8)
    movhs   \divisor, \divisor, lsr #8
    addhs   \order, \order, #8

    cmp     \divisor, #(1 << 4)
    movhs   \divisor, \divisor, lsr #4
    addhs   \order, \order, #4

    cmp     \divisor, #(1 << 2)
    addhi   \order, \order, #3
    addls   \order, \order, \divisor, lsr #1
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
    subs    r2, r1, #1
    bxeq    lr
    bcc     20f
    cmp     r0, r1
    bls     10f
    tst     r1, r2
    beq     30f

    ARM_DIV_BODY r0, r1, r2, r3
    mov     r0, r2
    bx      lr

10:
    moveq   r0, #1
20:
    movne   r0, #0
    bx      lr

30:
    ARM_DIV2_ORDER r1, r2
    mov     r0, r0, lsr r2
    bx      lr
