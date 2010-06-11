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

#if CONFIG_CPU == PP5002
    .section    .icode,"ax",%progbits
#else
    .text
#endif
    .align      2


    .global     lcd_write_data
    .type       lcd_write_data,%function

lcd_write_data:
    ldr     r12, =LCD1_BASE

.loop:
    ldrb    r2, [r0], #1

#ifdef IPOD_MINI2G
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    orr     r2, r2, #0x760000
1:
    ldr     r3, [r12]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [r12, #0x08]
#else
1:
    ldr     r3, [r12]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [r12, #0x10]

    ldrb    r2, [r0], #1
1:
    ldr     r3, [r12]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [r12, #0x10]
#endif

    subs    r1, r1, #1
    bne     .loop

    bx      lr
    .size   lcd_write_data,.-lcd_write_data
    

#ifdef IPOD_MINI2G

    .global     lcd_write_data_shifted
    .type       lcd_write_data_shifted,%function
    
lcd_write_data_shifted:
    stmfd   sp!, {r4, lr}
    ldr     lr, =LCD1_BASE
    mov     r12, #0x760000
    ldrb    r2, [r0], #1

.sloop:
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    mov     r4, r2, lsl #12
    orr     r4, r12, r4, lsr #16
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x08]

    subs    r1, r1, #1
    bne     .sloop

    ldmpc   regs=r4
    .size   lcd_write_data_shifted,.-lcd_write_data_shifted
    
#elif defined IPOD_MINI

    .global     lcd_write_data_shifted
    .type       lcd_write_data_shifted,%function
    
lcd_write_data_shifted:
    str     lr, [sp, #-4]!
    ldr     lr, =LCD1_BASE
    ldrb    r2, [r0], #1

.sloop:
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    mov     r12, r2, lsr #4
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r12, [lr, #0x10]

    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    mov     r12, r2, lsr #4
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r12, [lr, #0x10]

    subs    r1, r1, #1
    bne     .sloop

    ldrpc
    .size   lcd_write_data_shifted,.-lcd_write_data_shifted

#endif

    .global     lcd_mono_data
    .type       lcd_mono_data,%function
    
lcd_mono_data:
    stmfd   sp!, {r4, lr}
    ldr     lr, =LCD1_BASE
    adr     r12, .dibits

.mloop:
    ldrb    r2, [r0], #1
    mov     r3, r2, lsr #4
    ldrb    r4, [r12, r3]

#ifdef IPOD_MINI2G
    and     r3, r2, #0x0f
    ldrb    r3, [r12, r3]
    orr     r4, r3, r4, lsl #8
    orr     r4, r4, #0x760000
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x08]
#else
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x10]

    and     r3, r2, #0x0f
    ldrb    r4, [r12, r3]
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x10]
#endif

    subs    r1, r1, #1
    bne     .mloop

    ldmpc   regs=r4

.dibits:
    .byte   0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F
    .byte   0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF

    .size   lcd_mono_data,.-lcd_mono_data


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
 *   r5/r6 - lcd data accumulators
 *   r6/r7 - current block of values
 *   r12 - phase signs mask
 *   lr  - lcd bridge address
 */

lcd_grey_data:
    stmfd   sp!, {r4-r7, lr}
    mov     r12, #0x80
    orr     r12, r12, r12, lsl #8
    orr     r12, r12, r12, lsl #16
    ldr     lr, =LCD1_BASE

.greyloop:
    ldmia   r1, {r3-r4}         /* Fetch 8 pixel phases */

    bic     r5, r12, r3         /* r5 = 3.......2.......1.......0....... */
    orr     r5, r5, r5, lsr #10 /* r5 = 3.......2.3.....1.2.....0.1..... */
    orr     r5, r5, r5, lsr #10 /* r5 = 3.......2.3.....1.2.3...0.1.2... */
    orr     r5, r5, r5, lsr #10 /* r5 = 3.......2.3.....1.2.3...0.1.2.3. */
    orr     r5, r5, r5, lsr #1  /* r5 = 33......2233....112233..00112233 */
    bic     r3, r3, r12

#ifndef IPOD_MINI2G  /* 8 bit parallel bridge mode */
1:
    ldr     r6, [lr]
    tst     r6, #LCD1_BUSY_MASK
    bne     1b

    str     r5, [lr, #0x10]
#endif

    ldmia   r0!, {r6-r7}        /* Fetch 8 pixel values */
    add     r3, r3, r6

    bic     r6, r12, r4         /* r6 = 7.......6.......5.......4....... */
    orr     r6, r6, r6, lsr #10 /* r6 = 7.......6.7.....5.6.....4.5..... */
    orr     r6, r6, r6, lsr #10 /* r6 = 7.......6.7.....5.6.7...4.5.6... */
    orr     r6, r6, r6, lsr #10 /* r6 = 7.......6.7.....5.6.7...4.5.6.7. */
    orr     r6, r6, r6, lsr #1  /* r6 = 77......6677....556677..44556677 */
    bic     r4, r4, r12

    add     r4, r4, r7
    stmia   r1!, {r3-r4}

#ifdef IPOD_MINI2G   /* 16 bit serial bridge mode */
    and     r5, r5, #0xff       /* r5 = ........................00112233 */
    and     r6, r6, #0xff       /* r6 = ........................44556677 */
    orr     r5, r6, r5, lsl #8  /* r5 = ................0011223344556677 */
    orr     r5, r5, #0x760000   /* data marker */
#endif

1:
    ldr     r7, [lr]
    tst     r7, #LCD1_BUSY_MASK
    bne     1b

#ifdef IPOD_MINI2G
    str     r5, [lr, #0x08]
#else
    str     r6, [lr, #0x10]
#endif

    subs    r2, r2, #1
    bne     .greyloop

    ldmpc   regs=r4-r7
    .size   lcd_grey_data,.-lcd_grey_data

