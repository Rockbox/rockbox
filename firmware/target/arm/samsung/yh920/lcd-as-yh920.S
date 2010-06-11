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
 * Modified from ipod/lcd-as-gray.S for YH920 by Rafaël Carré
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


/* Writes an array of pixels of specified width (in 8 bits unit)
 *
 * Arguments:
 *  r0 : pixels array
 *  r1 : number of pixels in 8 bits unit
 *
 * Register usage:
 *  r2 : LCD_CONTROL value / pixel value
 *  r3 : lcd bridge address
 */

    .global     lcd_write_data
    .type       lcd_write_data,%function

lcd_write_data:
    ldr     r3, =LCD1_BASE

.loop:
1:
    ldr     r2, [r3]
    tst     r2, #LCD1_BUSY_MASK
    bne     1b

    ldrb    r2, [r0], #1
    str     r2, [r3, #0x10]

    subs    r1, r1, #1
    bne     .loop

    bx      lr
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
 *   r3 - current block of phases
 *   r4 - current block of values
 *   r5 - lcd data accumulator
 *   r12 - phase signs mask
 *   lr - lcd bridge address
 */

lcd_grey_data:
    stmfd   sp!, {r4-r5, lr}
    mov     r12, #0x80
    orr     r12, r12, r12, lsl #8
    orr     r12, r12, r12, lsl #16
    ldr     lr, =LCD1_BASE

.greyloop:
    ldr     r3, [r1]            /* Fetch 4 pixel phases */
    ldr     r4, [r0], #4        /* Fetch 4 pixel values */

    mov     r5, #0
    tst     r3, #0x80
    orreq   r5, r5, #0xc0
    tst     r3, #0x8000
    orreq   r5, r5, #0x30
    tst     r3, #0x800000
    orreq   r5, r5, #0x0c
    tst     r3, #0x80000000
    orreq   r5, r5, #0x03
    bic     r3, r3, r12
    add     r3, r3, r4
    str     r3, [r1], #4

1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b

    str     r5, [lr, #0x10]

    subs    r2, r2, #1
    bne     .greyloop

    ldmpc   regs=r4-r5
    .size   lcd_grey_data,.-lcd_grey_data

