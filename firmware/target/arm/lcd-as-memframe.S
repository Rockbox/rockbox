/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * ARM code for memory framebuffer LCDs
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

/****************************************************************************
 * void lcd_copy_buffer_rect(fb_data *dst, fb_data *src, int width,
 *                           int height);
 */
     .section    .icode, "ax", %progbits
     .align      2
     .global     lcd_copy_buffer_rect
     .type       lcd_copy_buffer_rect, %function
                                        @ r0 = dst
                                        @ r1 = src
                                        @ r2 = width
                                        @ r3 = height
lcd_copy_buffer_rect:                   @
    stmfd   sp!, { r4-r12, lr }         @ save non-scratch regs
    mov     r5, r2                      @ r5 = cached width
    rsb     r4, r2, #LCD_WIDTH          @ r4 = LCD_WIDTH - width
10: @ copy line                         @
    subs    r2, r5, #1                  @ r2 = width - 1
    beq     40f @ finish line           @ one halfword? skip to trailing copy
    tst     r0, #2                      @ word aligned?
    beq     20f @ rem copy              @ yes? skip to word copy
    ldrh    r6, [r1], #2                @ copy leading halfword
    subs    r2, r2, #1                  @
    strh    r6, [r0], #2                @
    ble     40f @ finish line           @ next line if lt or finish
                                        @ trailing halfword if eq
20: @ rem copy                          @
    add     r14, r2, #1                 @ get remaining width mod 16 after word
                                        @ align (rw)
    and     r14, r14, #0xe              @ r14 = 0 (16), 2, 4, 6, 8, 10, 12, 14
    add     pc, pc, r14, lsl #3         @ branch to 32-byte align
    nop                                 @ 
    b       30f                         @ rw % 16 = 0 or 1? use octword loop
    nop                                 @
    nop                                 @
    nop                                 @
    ldr     r6, [r1], #4                @ rw % 16 = 2 or 3
    subs    r2, r2, #2                  @
    str     r6, [r0], #4                @
    b       25f @ copy up done          @
    ldmia   r1!, { r6-r7 }              @ rw % 16 = 4 or 5
    subs    r2, r2, #4                  @
    stmia   r0!, { r6-r7 }              @
    b       25f @ copy up done          @
    ldmia   r1!, { r6-r8 }              @ rw % 16 = 6 or 7
    subs    r2, r2, #6                  @
    stmia   r0!, { r6-r8 }              @
    b       25f @ copy up done          @
    ldmia   r1!, { r6-r9 }              @ rw % 16 = 8 or 9
    subs    r2, r2, #8                  @
    stmia   r0!, { r6-r9 }              @
    b       25f @ copy up done          @
    ldmia   r1!, { r6-r10 }             @ rw % 16 = 10 or 11
    subs    r2, r2, #10                 @
    stmia   r0!, { r6-r10 }             @
    b       25f @ copy up done          @
    ldmia   r1!, { r6-r11 }             @ rw % 16 = 12 or 13
    subs    r2, r2, #12                 @
    stmia   r0!, { r6-r11 }             @
    b       25f @ copy up done          @
    ldmia   r1!, { r6-r12 }             @ rw % 16 = 14 or 15
    subs    r2, r2, #14                 @
    stmia   r0!, { r6-r12 }             @
25: @ copy up done                      @
    ble     40f @ finish line           @ no 32-byte segments remaining?
30: @ octword loop                      @ copy 16 pixels per loop
    ldmia   r1!, { r6-r12, r14 }        @
    subs    r2, r2, #16                 @
    stmia   r0!, { r6-r12, r14 }        @
    bgt     30b @ octword loop          @ 
40: @ finish line                       @
    ldreqh  r6, [r1], #2                @ finish last halfword if eq ...
    add     r1, r1, r4, lsl #1          @
    streqh  r6, [r0], #2                @ ...
    add     r0, r0, r4, lsl #1          @
    subs    r3, r3, #1                  @ next line
    bgt     10b @ copy line             @
    ldmfd   sp!, { r4-r12, pc }         @ restore regs and return
    .ltorg                              @ dump constant pool
    .size   lcd_copy_buffer_rect, .-lcd_copy_buffer_rect

/****************************************************************************
 * void lcd_write_yuv_420_lines(fb_data *dst,
 *                              unsigned char const * const src[3],
 *                              int width,
 *                              int stride);
 *
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
 *
 * Write four RGB565 pixels in the following order on each loop:
 * 1 3 + > down
 * 2 4 \/ left
 */
    .section    .icode, "ax", %progbits
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, %function
lcd_write_yuv420_lines:
                                        @ r0 = dst
                                        @ r1 = yuv_src
                                        @ r2 = width
                                        @ r3 = stride
    stmfd       sp!, { r4-r12 }         @ save non-scratch
    ldmia       r1, { r4, r5, r6 }      @ r4 = yuv_src[0] = Y'_p
                                        @ r5 = yuv_src[1] = Cb_p
                                        @ r6 = yuv_src[2] = Cr_p
                                        @ r1 = scratch
    sub         r3, r3, #1              @
10: @ loop line                         @
    ldrb        r7, [r4], #1            @ r7 = *Y'_p++;
    ldrb        r8, [r5], #1            @ r8 = *Cb_p++;
    ldrb        r9, [r6], #1            @ r9 = *Cr_p++;
                                        @
    sub         r7, r7, #16             @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @ actually (Y' - 16)*37 and shift right
    add         r7, r12, r7, asl #5     @ by one less when adding - same for all
                                        @
    sub         r8, r8, #128            @ Cb -= 128
    sub         r9, r9, #128            @ Cr -= 128
                                        @
    add         r10, r9, r9, asl #1     @ r10 = Cr*51 + Cb*24
    add         r10, r10, r10, asl #4   @
    add         r10, r10, r8, asl #3    @
    add         r10, r10, r8, asl #4    @
                                        @
    add         r11, r9, r9, asl #2     @ r9 = Cr*101
    add         r11, r11, r9, asl #5    @
    add         r9, r11, r9, asl #6     @
                                        @
    add         r8, r8, #2              @ r8 = bu = (Cb*128 + 128) >> 8
    mov         r8, r8, asr #2          @
    add         r9, r9, #256            @ r9 = rv = (r9 + 256) >> 9
    mov         r9, r9, asr #9          @
    rsb         r10, r10, #128          @ r10 = guv = (-r10 + 128) >> 8
    mov         r10, r10, asr #8        @
                                        @ compute R, G, and B
    add         r1, r8, r7, asr #8      @ r1  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r1, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r1, #31                 @ clamp b
    mvnhi       r1, r1, asr #31         @
    andhi       r1, r1, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r3]           @ r12 = Y' = *(Y'_p + stride)
                                        @
    orr         r1, r1, r7, lsl #5      @ r4 |= (g << 5)
    orr         r1, r1, r11, lsl #11    @ r4 = b | (r << 11)

#if LCD_WIDTH >= LCD_HEIGHT
    strh        r1, [r0]                @
#elif LCD_WIDTH < 256
    strh        r1, [r0], #LCD_WIDTH    @ store pixel
#else
    strh        r1, [r0]                @
#endif
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @
    add         r7, r12, r7, asl #5     @
                                        @ compute R, G, and B
    add         r1, r8, r7, asr #8      @ r1  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r1, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r1, #31                 @ clamp b
    mvnhi       r1, r1, asr #31         @
    andhi       r1, r1, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4], #1           @ r12 = Y' = *(Y'_p++)
                                        @
    orr         r1, r1, r11, lsl #11    @ r1 = b | (r << 11)
    orr         r1, r1, r7, lsl #5      @ r1 |= (g << 5)

#if LCD_WIDTH >= LCD_HEIGHT
    add         r0, r0, #2*LCD_WIDTH    @
    strh        r1, [r0]        @ store pixel
    sub         r0, r0, #2*LCD_WIDTH    @
#elif LCD_WIDTH < 256
    strh        r1, [r0, #-LCD_WIDTH-2] @ store pixel
#else
    strh        r1, [r0, #-2]           @
    add         r0, r0, #LCD_WIDTH      @
#endif
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @
    add         r7, r12, r7, asl #5     @
                                        @ compute R, G, and B
    add         r1, r8, r7, asr #8      @ r1  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r1, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r1, #31                 @ clamp b
    mvnhi       r1, r1, asr #31         @
    andhi       r1, r1, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r3]           @ r12 = Y' = *(Y'_p + stride)
                                        @
    orr         r1, r1, r7, lsl #5      @ r1 = b | (g << 5)
    orr         r1, r1, r11, lsl #11    @ r1 |= (r << 11)

#if LCD_WIDTH >= LCD_HEIGHT
    strh        r1, [r0, #2]
#elif LCD_WIDTH <  256
    strh        r1, [r0, #LCD_WIDTH]!   @ store pixel
#else
    strh        r1, [r0]                @
#endif
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @
    add         r7, r12, r7, asl #5     @
                                        @ compute R, G, and B
    add         r1, r8, r7, asr #8      @ r1  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r1, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r1, #31                 @ clamp b
    mvnhi       r1, r1, asr #31         @
    andhi       r1, r1, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    orr         r12, r1, r11, lsl #11   @ r12 = b | (r << 11)
    orr         r12, r12, r7, lsl #5    @ r12 |= (g << 5)

#if LCD_WIDTH >= LCD_HEIGHT
    add         r0, r0, #2*LCD_WIDTH
    strh        r12, [r0, #2]
#if LCD_WIDTH <= 512
    sub         r0, r0, #(2*LCD_WIDTH)-4
#else
    sub         r0, r0, #(2*LCD_WIDTH)
    add         r0, r0, #4
#endif
#else
    strh        r12, [r0, #-2]          @ store pixel
#if LCD_WIDTH < 256
    add         r0, r0, #2*LCD_WIDTH    @
#else
    add         r0, r0, #LCD_WIDTH      @
#endif
#endif
                                        @
    subs        r2, r2, #2              @ subtract block from width
    bgt         10b @ loop line         @
                                        @
    ldmfd       sp!, { r4-r12 }         @ restore registers and return
    bx          lr                      @
    .ltorg                              @ dump constant pool
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines


/****************************************************************************
 * void lcd_write_yuv_420_lines_odither(fb_data *dst,
 *                                      unsigned char const * const src[3],
 *                                      int width,
 *                                      int stride,
 *                                      int x_screen,
 *                                      int y_screen);
 *
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Red scaled at twice g & b but at same precision to place it in correct
 *   bit position after multiply and leave instruction count lower.
 *   |R|   |258   0  408| |Y' -  16|
 *   |G| = |149 -49 -104| |Cb - 128|
 *   |B|   |149 258    0| |Cr - 128|
 *
 * Write four RGB565 pixels in the following order on each loop:
 * 1 3 + > down
 * 2 4 \/ left
 *
 * Kernel pattern (raw|rotated|use order):
 * 5 3 4 2   2 6 3 7     row0   row2          > down
 * 1 7 0 6 | 4 0 5 1 | 2 4 6 0 3 5 7 1 col0     left
 * 4 2 5 3 | 3 7 2 6 | 3 5 7 1 2 4 6 0 col2  \/
 * 0 6 1 7   5 1 4 0
 */
    .section    .icode, "ax", %progbits
    .align      2
    .global     lcd_write_yuv420_lines_odither
    .type       lcd_write_yuv420_lines_odither, %function
lcd_write_yuv420_lines_odither:
                                        @ r0 = dst
                                        @ r1 = yuv_src
                                        @ r2 = width
                                        @ r3 = stride
                                        @ [sp]   = x_screen
                                        @ [sp+4] = y_screen
    stmfd       sp!, { r4-r12, lr }     @ save non-scratch
    ldmia       r1, { r4, r5, r6 }      @ r4 = yuv_src[0] = Y'_p
                                        @ r5 = yuv_src[1] = Cb_p
                                        @ r6 = yuv_src[2] = Cr_p
                                        @
    sub         r3, r3, #1              @
    add         r1, sp, #40             @ Line up pattern and kernel quadrant
    ldmia       r1, { r12, r14 }        @
    eor         r14, r14, r12           @
    and         r14, r14, #0x2          @
    mov         r14, r14, lsl #6        @ 0x00 or 0x80
10: @ loop line                         @
                                        @
    ldrb        r7, [r4], #1            @ r7 = *Y'_p++;
    ldrb        r8, [r5], #1            @ r8 = *Cb_p++;
    ldrb        r9, [r6], #1            @ r9 = *Cr_p++;
                                        @
    eor         r14, r14, #0x80         @ flip pattern quadrant
                                        @
    sub         r7, r7, #16             @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @    
    sub         r8, r8, #128            @ Cb -= 128
    sub         r9, r9, #128            @ Cr -= 128
                                        @
    add         r10, r8, r8, asl #4     @ r10 = guv = Cr*104 + Cb*49
    add         r10, r10, r8, asl #5    @
    add         r10, r10, r9, asl #3    @
    add         r10, r10, r9, asl #5    @
    add         r10, r10, r9, asl #6    @
                                        @
    mov         r8, r8, asl #1          @ r8 = bu = Cb*258
    add         r8, r8, r8, asl #7      @
                                        @
    add         r9, r9, r9, asl #1      @ r9 = rv = Cr*408
    add         r9, r9, r9, asl #4      @
    mov         r9, r9, asl #3          @
                                        @
                                        @ compute R, G, and B
    add         r1, r8, r7              @ r1  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
                                        @ r8 = bu, r9 = rv, r10 = guv
                                        @
    sub         r12, r1, r1, lsr #5     @ r1 = 31/32*b + b/256
    add         r1, r12, r1, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r + r/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7 = 63/64*g + g/256
    add         r7, r12, r7, lsr #8     @
                                        @
    add         r12, r14, #0x100        @
                                        @
    add         r1, r1, r12             @ b = r1 + delta
    add         r11, r11, r12, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r12, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r1, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r1, asr #15        @ clamp b
    mvnne       r1, r12, lsr #15        @
    andne       r1, r1, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r3]           @ r12 = Y' = *(Y'_p + stride)
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r1 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r1, r11, r1, lsr #10    @              (b >> 10)
                                        @
#if LCD_WIDTH >= LCD_HEIGHT
    strh        r1, [r0]        @
#elif LCD_WIDTH < 256
    strh        r1, [r0], #LCD_WIDTH    @ store pixel
#else
    strh        r1, [r0]                @
#endif
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @ compute R, G, and B
    add         r1, r8, r7              @ r1  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
    sub         r12, r1, r1, lsr #5     @ r1  = 31/32*b' + b'/256
    add         r1, r12, r1, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r' + r'/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7  = 63/64*g' + g'/256
    add         r7, r12, r7, lsr #8     @
                                        @
    add         r12, r14, #0x200        @
                                        @
    add         r1, r1, r12             @ b = r1 + delta
    add         r11, r11, r12, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r12, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r1, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r1, asr #15        @ clamp b
    mvnne       r1, r12, lsr #15        @
    andne       r1, r1, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4], #1           @ r12 = Y' = *(Y'_p++)
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r1 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r1, r11, r1, lsr #10    @              (b >> 10)
                                        @
#if LCD_WIDTH >= LCD_HEIGHT
    add         r0, r0, #2*LCD_WIDTH    @
    strh        r1, [r0]        @ store pixel
    sub         r0, r0, #2*LCD_WIDTH    @
#elif LCD_WIDTH < 256
    strh        r1, [r0, #-LCD_WIDTH-2] @ store pixel
#else
    strh        r1, [r0, #-2]           @ store pixel
    add         r0, r0, #LCD_WIDTH      @
#endif
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @ compute R, G, and B
    add         r1, r8, r7              @ r1  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
                                        @ r8 = bu, r9 = rv, r10 = guv
                                        @
    sub         r12, r1, r1, lsr #5     @ r1  = 31/32*b' + b'/256
    add         r1, r12, r1, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r' + r'/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7  = 63/64*g' + g'/256
    add         r7, r12, r7, lsr #8     @
                                        @
    add         r12, r14, #0x300        @
                                        @
    add         r1, r1, r12             @ b = r1 + delta
    add         r11, r11, r12, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r12, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r1, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r1, asr #15        @ clamp b
    mvnne       r1, r12, lsr #15        @
    andne       r1, r1, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r3]           @ r12 = Y' = *(Y'_p + stride)    
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r1 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r1, r11, r1, lsr #10    @              (b >> 10)
                                        @
#if LCD_WIDTH >= LCD_HEIGHT
    strh        r1, [r0, #2]
#elif LCD_WIDTH <  256
    strh        r1, [r0, #LCD_WIDTH]!   @ store pixel
#else
    strh        r1, [r0]                @
#endif

    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @ compute R, G, and B
    add         r1, r8, r7              @ r1  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
    sub         r12, r1, r1, lsr #5     @ r1 = 31/32*b + b/256
    add         r1, r12, r1, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r + r/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7 = 63/64*g + g/256
    add         r7, r12, r7, lsr #8     @
                                        @
    @ This element is zero - use r14    @
                                        @
    add         r1, r1, r14             @ b = r1 + delta
    add         r11, r11, r14, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r14, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r1, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r1, asr #15        @ clamp b
    mvnne       r1, r12, lsr #15        @
    andne       r1, r1, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r1 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r1, r11, r1, lsr #10    @              (b >> 10)
                                        @
#if LCD_WIDTH >= LCD_HEIGHT
    add         r0, r0, #2*LCD_WIDTH
    strh        r1, [r0, #2]            @ store pixel
#if LCD_WIDTH <= 512
    sub         r0, r0, #(2*LCD_WIDTH)-4
#else
    sub         r0, r0, #(2*LCD_WIDTH)
    add         r0, r0, #4
#endif
#else
    strh        r1, [r0, #-2]           @ store pixel
#if LCD_WIDTH < 256
    add         r0, r0, #2*LCD_WIDTH    @
#else
    add         r0, r0, #LCD_WIDTH      @
#endif
#endif
                                        @
    subs        r2, r2, #2              @ subtract block from width
    bgt         10b @ loop line         @
                                        @
    ldmfd       sp!, { r4-r12, pc }     @ restore registers and return
    .ltorg                              @ dump constant pool
    .size   lcd_write_yuv420_lines_odither, .-lcd_write_yuv420_lines_odither
