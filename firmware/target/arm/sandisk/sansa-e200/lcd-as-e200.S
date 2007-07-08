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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
    .size   lcd_copy_buffer_rect, .-lcd_copy_buffer_rect


/****************************************************************************
 * void lcd_write_yuv_420_lines(fb_data *dst,
 *                              unsigned char chroma_buf[LCD_HEIGHT/2*3],
                                unsigned char const * const src[3],
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
 */
    .section    .icode, "ax", %progbits
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, %function
lcd_write_yuv420_lines:
                                        @ r0   = dst
                                        @ r1   = chroma_buf
                                        @ r2   = yuv_src
                                        @ r3   = width
                                        @ [sp] = stride
    stmfd       sp!, { r4-r12, lr }     @ save non-scratch
    stmfd       sp!, { r0, r3 }         @ save dst and width
    mov         r14, #74                @ r14 = Y factor
    ldmia       r2, { r4, r5, r6 }      @ r4 = yuv_src[0] = Y'_p
                                        @ r5 = yuv_src[1] = Cb_p
                                        @ r6 = yuv_src[2] = Cr_p
10: @ loop line 1                       @
    ldrb        r2, [r4], #1            @ r2  = *Y'_p++;
    ldrb        r8, [r5], #1            @ r8  = *Cb_p++;
    ldrb        r11, [r6], #1           @ r11 = *Cr_p++;
                                        @
                                        @ compute Y
    sub         r2, r2, #16             @ r7 = Y = (Y' - 16)*74
    mul         r7, r2, r14             @
                                        @
    sub         r8, r8, #128            @ Cb -= 128
    sub         r11, r11, #128          @ Cr -= 128
                                        @
    mvn         r2, #24                 @ compute guv
    mul         r10, r2, r8             @ r10 = Cb*-24
    mvn         r2, #51                 @
    mla         r10, r2, r11, r10       @ r10 = r10 + Cr*-51
                                        @
    mov         r2, #101                @ compute rv
    mul         r9, r11, r2             @ r9 = rv = Cr*101
                                        @
                                        @ store chromas in line buffer
    add         r8, r8, #2              @ bu = (Cb + 2) >> 2
    mov         r8, r8, asr #2          @ 
    strb        r8, [r1], #1            @
    add         r9, r9, #256            @ rv = (Cr + 256) >> 9
    mov         r9, r9, asr #9          @
    strb        r9, [r1], #1            @
    mov         r10, r10, asr #8        @ guv >>= 8
    strb        r10, [r1], #1           @
                                        @ compute R, G, and B
    add         r2, r8, r7, asr #9      @ r2  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #9     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #8     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r2, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    mov         r12, #31                @
    cmp         r12, r2                 @ clamp b
    andlo       r2, r12, r2, asr #31    @
    eorlo       r2, r2, r12             @
    cmp         r12, r11                @ clamp r
    andlo       r11, r12, r11, asr #31  @
    eorlo       r11, r11, r12           @
    cmp         r12, r7, asr #1         @ clamp g
    andlo       r7, r12, r7, asr #31    @
    eorlo       r7, r7, r12             @
    orrlo       r7, r7, r7, asl #1      @
15: @ no clamp                          @
                                        @
    orr         r12, r2, r7, lsl #5     @ r4 |= (g << 5)
    ldrb        r2, [r4], #1            @ r2 = Y' = *Y'_p++
    orr         r12, r12, r11, lsl #11  @ r4 = b | (r << 11)
    strh        r12, [r0], #LCD_WIDTH   @ store pixel
                                        @
    sub         r2, r2, #16             @ r7 = Y = (Y' - 16)*74
    mul         r7, r2, r14             @ next Y
                                        @ compute R, G, and B
    add         r2, r8, r7, asr #9      @ r2  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #9     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #8     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r2, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    mov         r12, #31                @
    cmp         r12, r2                 @ clamp b
    andlo       r2, r12, r2, asr #31    @
    eorlo       r2, r2, r12             @
    cmp         r12, r11                @ clamp r
    andlo       r11, r12, r11, asr #31  @
    eorlo       r11, r11, r12           @
    cmp         r12, r7, asr #1         @ clamp g
    andlo       r7, r12, r7, asr #31    @
    eorlo       r7, r7, r12             @
    orrlo       r7, r7, r7, asl #1      @
15: @ no clamp                          @
                                        @
    orr         r12, r2, r11, lsl #11   @ r4 = b | (r << 11)
    orr         r12, r12, r7, lsl #5    @ r4 |= (g << 5)
    strh        r12, [r0, #LCD_WIDTH]!  @ store pixel
    add         r0, r0, #2*LCD_WIDTH    @
                                        @
    subs        r3, r3, #2              @
    bgt         10b @ loop line 1       @
                                        @ do second line
                                        @
    ldmfd       sp!, { r0, r3 }         @ pop dst and width
    sub         r0, r0, #2              @ set dst to start of next line
    sub         r1, r1, r3, asl #1      @ rewind chroma pointer...
    ldr         r2, [sp, #40]           @ r2 = stride
    add         r1, r1, r3, asr #1      @ ... (r1 -= width/2*3)
                                        @ move sources to start of next line
    sub         r2, r2, r3              @ r2 = skip = stride - width
    add         r4, r4, r2              @ r4 = Y'_p + skip
                                        @
20: @ loop line 2                       @
    ldrb        r2, [r4], #1            @ r7 = Y' = *Y'_p++
    ldrsb       r8, [r1], #1            @ reload saved chromas
    ldrsb       r9, [r1], #1            @
    ldrsb       r10, [r1], #1           @
                                        @
    sub         r2, r2, #16             @ r2 = Y = (Y' - 16)*74
    mul         r7, r2, r14             @
                                        @ compute R, G, and B
    add         r2, r8, r7, asr #9      @ r2  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #9     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #8     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r2, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         25f @ no clamp          @
    mov         r12, #31                @
    cmp         r12, r2                 @ clamp b
    andlo       r2, r12, r2, asr #31    @
    eorlo       r2, r2, r12             @
    cmp         r12, r11                @ clamp r
    andlo       r11, r12, r11, asr #31  @
    eorlo       r11, r11, r12           @
    cmp         r12, r7, asr #1         @ clamp g
    andlo       r7, r12, r7, asr #31    @
    eorlo       r7, r7, r12             @
    orrlo       r7, r7, r7, asl #1      @
25: @ no clamp                          @
                                        @
    orr         r12, r2, r11, lsl #11   @ r4 = b | (r << 11)
    ldrb        r2, [r4], #1            @ r2 = Y' = *Y'_p++
    orr         r12, r12, r7, lsl #5    @ r4 |= (g << 5)
    strh        r12, [r0], #LCD_WIDTH   @ store pixel
                                        @
                                        @ do second pixel
                                        @
    sub         r2, r2, #16             @ r2 = Y = (Y' - 16)*74
    mul         r7, r2, r14             @
                                        @ compute R, G, and B
    add         r2, r8, r7, asr #9      @ r2  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #9     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #8     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r2, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         25f @ no clamp          @
    mov         r12, #31                @
    cmp         r12, r2                 @ clamp b
    andlo       r2, r12, r2, asr #31    @
    eorlo       r2, r2, r12             @
    cmp         r12, r11                @ clamp r
    andlo       r11, r12, r11, asr #31  @
    eorlo       r11, r11, r12           @
    cmp         r12, r7, asr #1         @ clamp g
    andlo       r7, r12, r7, asr #31    @
    eorlo       r7, r7, r12             @
    orrlo       r7, r7, r7, asl #1      @
25: @ no clamp                          @
                                        @
    orr         r12, r2, r11, lsl #11   @ r4 = b | (r << 11)
    orr         r12, r12, r7, lsl #5    @ r4 |= (g << 5)
    strh        r12, [r0, #LCD_WIDTH]!  @ store pixel
    add         r0, r0, #2*LCD_WIDTH    @
                                        @
    subs        r3, r3, #2              @
    bgt         20b @ loop line 2       @
                                        @
    ldmfd       sp!, { r4-r12, pc }     @ restore registers and return
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines
