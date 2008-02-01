/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2008 by Michael Sevakis
 *
 * H10 20GB LCD assembly routines
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
 * void lcd_write_yuv_420_lines(unsigned char const * const src[3],
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
                                        @ r0 = yuv_src
                                        @ r1 = width
                                        @ r2 = stride
    stmfd       sp!, { r4-r12, lr }     @ save non-scratch
    ldmia       r0, { r4, r5, r6 }      @ r4 = yuv_src[0] = Y'_p
                                        @ r5 = yuv_src[1] = Cb_p
                                        @ r6 = yuv_src[2] = Cr_p
                                        @
    mov         r0, #0x7000000c         @ r0 = &LCD2_PORT = 0x70008a0c
    add         r0, r0, #0x8a00         @
    mov         r14, #LCD2_DATA_MASK    @
                                        @
    sub         r2, r2, #1              @ Adjust stride because of increment
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
    add         r9, r9, #256            @ r9 = rv = (r8 + 256) >> 9
    mov         r9, r9, asr #9          @
    rsb         r10, r10, #128          @ r10 = guv = (-r9 + 128) >> 8
    mov         r10, r10, asr #8        @
                                        @ compute R, G, and B
    add         r3, r8, r7, asr #8      @ r3 = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r3, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r3, #31                 @ clamp b
    mvnhi       r3, r3, asr #31         @
    andhi       r3, r3, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r2]           @ r12 = Y' = *(Y'_p + stride)
                                        @
    orr         r3, r3, r11, lsl #11    @ r3 = b | (r << 11)
    orr         r3, r3, r7, lsl #5      @ r3 |= (g << 5)
                                        @
    orr         r7, r14, r3, lsr #8     @ store pixel
    orr         r11, r14, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @
    add         r7, r12, r7, asl #5     @
                                        @ compute R, G, and B
    add         r3, r8, r7, asr #8      @ r3  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r3, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r3, #31                 @ clamp b
    mvnhi       r3, r3, asr #31         @
    andhi       r3, r3, #31             @
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
    orr         r3, r3, r11, lsl #11    @ r3 = b | (r << 11)
    orr         r3, r3, r7, lsl #5      @ r3 |= (g << 5)
                                        @
    orr         r7, r14, r3, lsr #8     @ store pixel
    orr         r11, r14, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @
    add         r7, r12, r7, asl #5     @
                                        @ compute R, G, and B
    add         r3, r8, r7, asr #8      @ r3  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r3, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r3, #31                 @ clamp b
    mvnhi       r3, r3, asr #31         @
    andhi       r3, r3, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r2]           @ r12 = Y' = *(Y'_p + stride)
                                        @
    orr         r3, r3, r7, lsl #5      @ r3 = b | (g << 5)
    orr         r3, r3, r11, lsl #11    @ r3 |= (r << 11)
                                        @
    orr         r7, r14, r3, lsr #8     @ store pixel
    orr         r11, r14, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*74
    add         r12, r7, r7, asl #2     @
    add         r7, r12, r7, asl #5     @
                                        @ compute R, G, and B
    add         r3, r8, r7, asr #8      @ r3  = b = (Y >> 9) + bu
    add         r11, r9, r7, asr #8     @ r11 = r = (Y >> 9) + rv
    add         r7, r10, r7, asr #7     @ r7  = g = (Y >> 8) + guv
                                        @
    orr         r12, r3, r11            @ check if clamping is needed...
    orr         r12, r12, r7, asr #1    @ ...at all
    cmp         r12, #31                @
    bls         15f @ no clamp          @
    cmp         r3, #31                 @ clamp b
    mvnhi       r3, r3, asr #31         @
    andhi       r3, r3, #31             @
    cmp         r11, #31                @ clamp r
    mvnhi       r11, r11, asr #31       @
    andhi       r11, r11, #31           @
    cmp         r7, #63                 @ clamp g
    mvnhi       r7, r7, asr #31         @
    andhi       r7, r7, #63             @
15: @ no clamp                          @
                                        @
    orr         r3, r3, r11, lsl #11    @ r3 = b | (r << 11)
    orr         r3, r3, r7, lsl #5      @ r3 |= (g << 5)
                                        @
    orr         r7, r14, r3, lsr #8     @ store pixel
    orr         r11, r14, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    subs        r1, r1, #2              @ subtract block from width
    bgt         10b @ loop line         @
                                        @
    ldmfd       sp!, { r4-r12, pc }     @ restore registers and return
    .ltorg                              @ dump constant pool
    .size   lcd_write_yuv420_lines, .-lcd_write_yuv420_lines


/****************************************************************************
 * void lcd_write_yuv_420_lines_odither(unsigned char const * const src[3],
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
 * Kernel pattern (raw|use order):
 * 5 3 4 2     row0    row2         > down
 * 1 7 0 6 | 5 1 3 7 4 0 2 6 col0     left
 * 4 2 5 3 | 4 0 2 6 5 1 3 7 col2  \/
 * 0 6 1 7
 */
    .section    .icode, "ax", %progbits
    .align      2
    .global     lcd_write_yuv420_lines_odither
    .type       lcd_write_yuv420_lines_odither, %function
lcd_write_yuv420_lines_odither:
                                        @ r0   = yuv_src
                                        @ r1   = width
                                        @ r2   = stride
                                        @ r3   = x_screen
                                        @ [sp] = y_screen
    stmfd       sp!, { r4-r12, lr }     @ save non-scratch
    ldmia       r0, { r4, r5, r6 }      @ r4 = yuv_src[0] = Y'_p
                                        @ r5 = yuv_src[1] = Cb_p
                                        @ r6 = yuv_src[2] = Cr_p
                                        @
    ldr         r0, [sp, #40]           @ Line up pattern and kernel quadrant
    eor         r14, r3, r0             @
    and         r14, r14, #0x2          @
    mov         r14, r14, lsl #6        @ 0x00 or 0x80
                                        @
    mov         r0, #0x7000000c         @ r0 = &LCD2_PORT = 0x70008a0c
    add         r0, r0, #0x8a00         @
                                        @
    sub         r2, r2, #1              @ Adjust stride because of increment
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
    add         r3, r8, r7              @ r3  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
                                        @ r8 = bu, r9 = rv, r10 = guv
                                        @
    sub         r12, r3, r3, lsr #5     @ r3 = 31/32*b + b/256
    add         r3, r12, r3, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r + r/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7 = 63/64*g + g/256
    add         r7, r12, r7, lsr #8     @
                                        @
    add         r12, r14, #0x200        @
                                        @
    add         r3, r3, r12             @ b = r3 + delta
    add         r11, r11, r12, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r12, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r3, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r3, asr #15        @ clamp b
    mvnne       r3, r12, lsr #15        @
    andne       r3, r3, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r2]           @ r12 = Y' = *(Y'_p + stride)
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r3 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r3, r11, r3, lsr #10    @              (b >> 10)
                                        @
    mov         r11, #LCD2_DATA_MASK    @ store pixel
    orr         r7, r11, r3, lsr #8     @
    orr         r11, r11, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @ compute R, G, and B
    add         r3, r8, r7              @ r3  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
    sub         r12, r3, r3, lsr #5     @ r3  = 31/32*b' + b'/256
    add         r3, r12, r3, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r' + r'/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7  = 63/64*g' + g'/256
    add         r7, r12, r7, lsr #8     @
                                        @
    @ This element is zero - use r14    @
                                        @
    add         r3, r3, r14             @ b = r3 + delta
    add         r11, r11, r14, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r14, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r3, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r3, asr #15        @ clamp b
    mvnne       r3, r12, lsr #15        @
    andne       r3, r3, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4], #1           @ r12 = Y' = *(Y'_p++)
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r3 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r3, r11, r3, lsr #10    @              (b >> 10)
                                        @
    mov         r11, #LCD2_DATA_MASK    @ store pixel
    orr         r7, r11, r3, lsr #8     @
    orr         r11, r11, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @ compute R, G, and B
    add         r3, r8, r7              @ r3  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
                                        @ r8 = bu, r9 = rv, r10 = guv
                                        @
    sub         r12, r3, r3, lsr #5     @ r3  = 31/32*b' + b'/256
    add         r3, r12, r3, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r' + r'/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7  = 63/64*g' + g'/256
    add         r7, r12, r7, lsr #8     @
                                        @
    add         r12, r14, #0x100        @
                                        @
    add         r3, r3, r12             @ b = r3 + delta
    add         r11, r11, r12, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r12, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r3, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r3, asr #15        @ clamp b
    mvnne       r3, r12, lsr #15        @
    andne       r3, r3, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    ldrb        r12, [r4, r2]           @ r12 = Y' = *(Y'_p + stride)    
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r3 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r3, r11, r3, lsr #10    @              (b >> 10)
                                        @
    mov         r11, #LCD2_DATA_MASK    @ store pixel
    orr         r7, r11, r3, lsr #8     @
    orr         r11, r11, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    sub         r7, r12, #16            @ r7 = Y = (Y' - 16)*149
    add         r12, r7, r7, asl #2     @
    add         r12, r12, r12, asl #4   @
    add         r7, r12, r7, asl #6     @
                                        @ compute R, G, and B
    add         r3, r8, r7              @ r3  = b' = Y + bu
    add         r11, r9, r7, asl #1     @ r11 = r' = Y*2 + rv
    rsb         r7, r10, r7             @ r7  = g' = Y + guv
                                        @
    sub         r12, r3, r3, lsr #5     @ r3 = 31/32*b + b/256
    add         r3, r12, r3, lsr #8     @
                                        @
    sub         r12, r11, r11, lsr #5   @ r11 = 31/32*r + r/256
    add         r11, r12, r11, lsr #8   @
                                        @
    sub         r12, r7, r7, lsr #6     @ r7 = 63/64*g + g/256
    add         r7, r12, r7, lsr #8     @
                                        @
    add         r12, r14, #0x300        @
                                        @
    add         r3, r3, r12             @ b = r3 + delta
    add         r11, r11, r12, lsl #1   @ r = r11 + delta*2
    add         r7, r7, r12, lsr #1     @ g = r7 + delta/2
                                        @
    orr         r12, r3, r11, asr #1    @ check if clamping is needed...
    orr         r12, r12, r7            @ ...at all
    movs        r12, r12, asr #15       @
    beq         15f @ no clamp          @
    movs        r12, r3, asr #15        @ clamp b
    mvnne       r3, r12, lsr #15        @
    andne       r3, r3, #0x7c00         @ mask b only if clamped
    movs        r12, r11, asr #16       @ clamp r
    mvnne       r11, r12, lsr #16       @
    movs        r12, r7, asr #15        @ clamp g
    mvnne       r7, r12, lsr #15        @
15: @ no clamp                          @
                                        @
    and         r11, r11, #0xf800       @ pack pixel
    and         r7, r7, #0x7e00         @ r3 = pixel = (r & 0xf800) |
    orr         r11, r11, r7, lsr #4    @              ((g & 0x7e00) >> 4) |
    orr         r3, r11, r3, lsr #10    @              (b >> 10)
                                        @
    mov         r11, #LCD2_DATA_MASK    @ store pixel
    orr         r7, r11, r3, lsr #8     @
    orr         r11, r11, r3            @
20:                                     @
    ldr         r3, [r0]                @
    tst         r3, #LCD2_BUSY_MASK     @
    bne         20b                     @
    str         r7, [r0]                @
    str         r11, [r0]               @
                                        @
    subs        r1, r1, #2              @ subtract block from width
    bgt         10b @ loop line         @
                                        @
    ldmfd       sp!, { r4-r12, pc }     @ restore registers and return
    .ltorg                              @ dump constant pool
    .size   lcd_write_yuv420_lines_odither, .-lcd_write_yuv420_lines_odither
