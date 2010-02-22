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

#include "as3525.h"

    .text
    .align      2

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
 *   r12 - phase signs mask
 *   lr - lcd bridge address
 */

lcd_grey_data:
    stmfd   sp!, {r4-r7, lr}
    mov     r12, #0x80
    orr     r12, r12, r12, lsl #8
    orr     r12, r12, r12, lsl #16

    ldr     lr, =GPIOB_BASE
    mov     r3, #(1<<2)
    str     r3, [lr, #(4*(1<<2))] @ set pin D/C# of LCD controller (data)

    ldr     lr, =SSP_BASE

.greyloop:
    ldmia   r1, {r3-r4}         /* Fetch 8 pixel phases */
    ldmia   r0!, {r5-r6}        /* Fetch 8 pixel values */

    mov     r7, #0

    /* set bits 7..4 */
    tst     r3, #0x80
    orrne   r7, r7, #0x80
    tst     r3, #0x8000
    orrne   r7, r7, #0x40
    tst     r3, #0x800000
    orrne   r7, r7, #0x20
    tst     r3, #0x80000000
    orrne   r7, r7, #0x10
    bic     r3, r3, r12
    add     r3, r3, r5

    /* set bits 3..0 */
    tst     r4, #0x80
    orrne   r7, r7, #0x08
    tst     r4, #0x8000
    orrne   r7, r7, #0x04
    tst     r4, #0x800000
    orrne   r7, r7, #0x02
    tst     r4, #0x80000000
    orrne   r7, r7, #0x01
    bic     r4, r4, r12
    add     r4, r4, r6

    stmia   r1!, {r3-r4}

1:
    ldr     r5, [lr, #0xC]      @ SSP_SR
    ands    r5, r5, #(1<<1)     @ wait until transmit fifo isn't full
    beq     1b

    strb    r7, [lr, #0x08]     @ SSP_DATA

    subs    r2, r2, #1
    bne     .greyloop

    ldmfd   sp!, {r4-r7, pc}
    .size   lcd_grey_data,.-lcd_grey_data
