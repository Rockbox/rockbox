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

.macro ARM_DIV2_ORDER divisor, order

    @ There's exactly one bit set in the divisor, so ffs() can be used
    @ This is the ffs algorithm devised by D.Seal and posted to
    @ comp.sys.arm on 16 Feb 1994.
    adr     \order, L_ffs_table
    orr     \divisor, \divisor, \divisor, lsl #4   @  = X * 0x11
    orr     \divisor, \divisor, \divisor, lsl #6   @  = X * 0x451
    rsb     \divisor, \divisor, \divisor, lsl #16  @  = X * 0x0450fbaf

    ldrb    \order, [\order, \divisor, lsr #26]
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

L_ffs_table:
    @        0   1   2   3   4   5   6   7
    @----------------------------------------------
    .byte   32,  0,  1, 12,  2,  6,  0, 13  @  0- 7
    .byte    3,  0,  7,  0,  0,  0,  0, 14  @  8-15
    .byte   10,  4,  0,  0,  8,  0,  0, 25  @ 16-23
    .byte    0,  0,  0,  0,  0, 21, 27, 15  @ 24-31
    .byte   31, 11,  5,  0,  0,  0,  0,  0  @ 32-39
    .byte    9,  0,  0, 24,  0,  0, 20, 26  @ 40-47
    .byte   30,  0,  0,  0,  0, 23,  0, 19  @ 48-55
    .byte   29,  0, 22, 18, 28, 17, 16,  0  @ 56-63
