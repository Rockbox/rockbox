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
    str     lr, [sp, #-4]!
    ldr     lr, =LCD1_BASE

.loop:
    ldrb    r2, [r0], #1

#ifdef IPOD_MINI2G
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    orr     r2, r2, #0x760000
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [lr, #0x08]
#else
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [lr, #0x10]

    ldrb    r2, [r0], #1
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r2, [lr, #0x10]
#endif

    subs    r1, r1, #1
    bne     .loop

    ldr     pc, [sp], #4
    .size   lcd_write_data,.-lcd_write_data
    

#ifdef IPOD_MINI2G

    .global     lcd_write_data_shifted
    .type       lcd_write_data_shifted,%function
    
lcd_write_data_shifted:
    stmfd   sp!, {r4, r5, lr}
    ldr     lr, =LCD1_BASE
    mov     r5, #0x760000
    ldrb    r2, [r0], #1

.sloop:
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    mov     r4, r2, lsl #12
    orr     r4, r5, r4, lsr #16
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x08]

    subs    r1, r1, #1
    bne     .sloop

    ldmfd   sp!, {r4, r5, pc}
    .size   lcd_write_data_shifted,.-lcd_write_data_shifted
    
#elif defined IPOD_MINI

    .global     lcd_write_data_shifted
    .type       lcd_write_data_shifted,%function
    
lcd_write_data_shifted:
    stmfd   sp!, {r4, lr}
    ldr     lr, =LCD1_BASE
    ldrb    r2, [r0], #1

.sloop:
    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    mov     r4, r2, lsr #4
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x10]

    ldrb    r3, [r0], #1
    orr     r2, r3, r2, lsl #8
    mov     r4, r2, lsr #4
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x10]

    subs    r1, r1, #1
    bne     .sloop

    ldmfd   sp!, {r4, pc}
    .size   lcd_write_data_shifted,.-lcd_write_data_shifted

#endif

    .global     lcd_mono_data
    .type       lcd_mono_data,%function
    
lcd_mono_data:
    stmfd   sp!, {r4, r5, lr}
    ldr     lr, =LCD1_BASE
    adr     r5, .dibits

.mloop:
    ldrb    r2, [r0], #1
    mov     r3, r2, lsr #4
    ldrb    r4, [r5, r3]

#ifdef IPOD_MINI2G
    and     r3, r2, #0x0f
    ldrb    r3, [r5, r3]
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
    ldrb    r4, [r5, r3]
1:
    ldr     r3, [lr]
    tst     r3, #LCD1_BUSY_MASK
    bne     1b
    str     r4, [lr, #0x10]
#endif

    subs    r1, r1, #1
    bne     .mloop

    ldmfd   sp!, {r4, r5, pc}

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

#ifdef IPOD_MINI2G              /* Serial bridge mode */
    mov     r7, #0x760000
    tst     r3, #0x80
    orreq   r7, r7, #0xc000
    tst     r3, #0x8000
    orreq   r7, r7, #0x3000
    tst     r3, #0x800000
    orreq   r7, r7, #0x0c00
    tst     r3, #0x80000000
    orreq   r7, r7, #0x0300
    bic     r3, r3, r8
    add     r3, r3, r5
#else                           /* Parallel bridge mode */
    mov     r7, #0
    tst     r3, #0x80
    orreq   r7, r7, #0xc0
    tst     r3, #0x8000
    orreq   r7, r7, #0x30
    tst     r3, #0x800000
    orreq   r7, r7, #0x0c
    tst     r3, #0x80000000
    orreq   r7, r7, #0x03
    bic     r3, r3, r8
    add     r3, r3, r5

1:
    ldr     r5, [lr]
    tst     r5, #LCD1_BUSY_MASK
    bne     1b

    str     r7, [lr, #0x10]
    mov     r7, #0
#endif

    tst     r4, #0x80
    orreq   r7, r7, #0xc0
    tst     r4, #0x8000
    orreq   r7, r7, #0x30
    tst     r4, #0x800000
    orreq   r7, r7, #0x0c
    tst     r4, #0x80000000
    orreq   r7, r7, #0x03
    bic     r4, r4, r8
    add     r4, r4, r6

    stmia   r1!, {r3-r4}

1:
    ldr     r5, [lr]
    tst     r5, #LCD1_BUSY_MASK
    bne     1b
#ifdef IPOD_MINI2G
    str     r7, [lr, #0x08]
#else
    str     r7, [lr, #0x10]
#endif

    subs    r2, r2, #1
    bne     .greyloop

    ldmfd   sp!, {r4-r8, pc}
    .size   lcd_grey_data,.-lcd_grey_data

