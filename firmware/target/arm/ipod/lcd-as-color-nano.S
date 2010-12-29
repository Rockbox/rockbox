/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 by Andree Buschmann
 *
 * Generic asm helper function used by YUV blitting.
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

    .section    .icode, "ax", %progbits

/****************************************************************************
*  void lcd_yuv_write_inner_loop(unsigned char const * const ysrc,
*                                unsigned char const * const usrc,
*                                unsigned char const * const vsrc,
*                                int width);
*
*   YUV- > RGB565 conversion
*   |R|   |1.000000 -0.000001  1.402000| |Y'|
*   |G| = |1.000000 -0.334136 -0.714136| |Pb|
*   |B|   |1.000000  1.772000  0.000000| |Pr|
*   Scaled, normalized, rounded and tweaked to yield RGB 565:
*   |R|   |74   0 101| |Y' -  16| >> 9
*   |G| = |74 -24 -51| |Cb - 128| >> 8
*   |B|   |74 128   0| |Cr - 128| >> 9
*
*/
    .align      2
    .global     lcd_yuv_write_inner_loop
    .type       lcd_yuv_write_inner_loop, %function

lcd_yuv_write_inner_loop:
                                        @ r0 = ysrc
                                        @ r1 = usrc
                                        @ r2 = vsrc
                                        @ r3 = width
    stmfd sp!, { r4-r11, lr }           @ save regs
    mov r4, #0x70000000                 @ r4 = LCD2_BLOCK_CTRL - 0x20
    add r4, r4, #0x8a00                 @
    add r5, r4, #0x100                  @ r5 = LCD2_BLOCK_DATA
10:                                     @ loop

    ldrb r7, [r1], #1                   @ *usrc++
    ldrb r8, [r2], #1                   @ *vsrc++

    sub r7, r7, #128                    @ Cb -= 128
    sub r8, r8, #128                    @ Cr -= 128

    add r10, r8, r8, asl #2             @ Cr*101
    add r10, r10, r8, asl #5
    add r10, r10, r8, asl #6

    add r11, r8, r8, asl #1             @ Cr*51 + Cb*24
    add r11, r11, r11, asl #4
    add r11, r11, r7, asl #3
    add r11, r11, r7, asl #4

    add r12, r7, #2                     @ r12 = bu = (Cb*128 + 256) >> 9
    mov r12, r12, asr #2
    add r10, r10, #256                  @ r10 = rv = (Cr*101 + 256) >> 9
    mov r10, r10, asr #9
    rsb r11, r11, #128                  @ r11 = guv = (-r11 + 128) >> 8
    mov r11, r11, asr #8

@ pixel_1
    ldrb r7, [r0], #1                   @ *ysrc++
    sub r7, r7, #16                     @ Y = (Y' - 16) * 37
    add r8, r7, r7, asl #2
    add r7, r8, r7, asl #5

    add r9, r10, r7, asr #8             @ R = (Y >> 8) + rv
    add r8, r11, r7, asr #7             @ G = (Y >> 7) + guv
    add r7, r12, r7, asr #8             @ B = (Y >> 8) + bu

    cmp r9, #31                         @ clamp R
    mvnhi r9, r9, asr #31
    andhi r9, r9, #31

    cmp r8, #63                         @ clamp G
    mvnhi r8, r8, asr #31
    andhi r8, r8, #63

    cmp r7, #31                         @ clamp B
    mvnhi r7, r7, asr #31
    andhi r7, r7, #31

    orr r6, r7, r8, lsl #5              @ pack pixel
    orr r6, r6, r9, lsl #11

    mov r7, r6, lsl #8                  @ swap bytes
    and r7, r7, #0xff00
    add r6, r7, r6, lsr #8

@ pixel_2
    ldrb r7, [r0], #1                   @ *ysrc++
    sub r7, r7, #16                     @ Y = (Y' - 16) * 37
    add r8, r7, r7, asl #2
    add r7, r8, r7, asl #5

    add r9, r10, r7, asr #8             @ R = (Y >> 8) + rv
    add r8, r11, r7, asr #7             @ G = (Y >> 7) + guv
    add r7, r12, r7, asr #8             @ B = (Y >> 8) + bu

    cmp r9, #31                         @ clamp R
    mvnhi r9, r9, asr #31
    andhi r9, r9, #31

    cmp r8, #63                         @ clamp G
    mvnhi r8, r8, asr #31
    andhi r8, r8, #63

    cmp r7, #31                         @ clamp B
    mvnhi r7, r7, asr #31
    andhi r7, r7, #31

    orr r7, r7, r8, lsl #5              @ pack pixel
    orr r7, r7, r9, lsl #11

    orr r6, r6, r7, lsl #24             @ swap bytes and add pixels simultaneously
    mov r7, r7, lsr #8
    orr r6, r6, r7, lsl #16
#if 1
11:                                     @ while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_TXOK));
    ldr r11, [r4, #0x20]                @
    tst r11, #0x1000000                 @
    beq 11b                             @
#endif
    str r6, [r5]                        @ send two pixels

    subs r3, r3, #2                     @ decrease width
    bgt 10b                             @ loop

    ldmpc regs=r4-r11                   @ restore regs
    .ltorg                              @ dump constant pool
    .size   lcd_yuv_write_inner_loop, .-lcd_yuv_write_inner_loop
