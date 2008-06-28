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
#include "cpu.h"

    .text
    .align      2


    .global     lcd_write_data
    .type       lcd_write_data,%function

lcd_write_data:
    str     lr, [sp, #-4]!
    ldr     lr, =LCD1_BASE

.loop:
    ldrb    r2, [r0], #1
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [lr, #0x10]

    subs    r1, r1, #1
    bne     .loop

    ldr     pc, [sp], #4
    .size   lcd_write_data,.-lcd_write_data


    .global     lcd_grey_data
    .type       lcd_grey_data,%function

/* A high performance function to write grey phase data to the display,
 * one or multiple pixels.
 *
 * Arguments:
 *   r0 - pixel value data address
 *   r1 - pixel phase data address
 *   r2 - pixel block count
 *
 * Register usage:
 *   r3/r4 - current block of phases
 *   r5/r6 - current block of values
 *   r7 - lcd data accumulator
 *   r8 - phase signs mask
 *   lr - lcd bridge address
 */
 
lcd_grey_data:
    stmfd   sp!, {r4-r8, lr}
    mov     r8, #0x80
    orr     r8, r8, r8, lsl #8
    orr     r8, r8, r8, lsl #16
    ldr     lr, =LCD1_BASE

.greyloop:
    ldmia   r1, {r3-r4}         /* Fetch 8 pixel phases */
    ldmia   r0!, {r5-r6}        /* Fetch 8 pixel values */

    mov     r7, #0xff
    tst     r3, #0x80
    biceq   r7, r7, #0x80
    tst     r3, #0x8000
    biceq   r7, r7, #0x40
    tst     r3, #0x800000
    biceq   r7, r7, #0x20
    tst     r3, #0x80000000
    biceq   r7, r7, #0x10
    bic     r3, r3, r8
    add     r3, r3, r5

    tst     r4, #0x80
    biceq   r7, r7, #0x08
    tst     r4, #0x8000
    biceq   r7, r7, #0x04
    tst     r4, #0x800000
    biceq   r7, r7, #0x02
    tst     r4, #0x80000000
    biceq   r7, r7, #0x01
    bic     r4, r4, r8
    add     r4, r4, r6

    stmia   r1!, {r3-r4}

1:
    ldr     r5, [lr]
    tst     r5, #LCD1_BUSY_MASK
    bne     1b
    str     r7, [lr, #0x10]

    subs    r2, r2, #1
    bne     .greyloop

    ldmfd   sp!, {r4-r8, pc}
    .size   lcd_grey_data,.-lcd_grey_data

