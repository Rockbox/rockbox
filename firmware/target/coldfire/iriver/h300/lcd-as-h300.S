/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
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

    .section    .icode, "ax", @progbits

    .align      2
    .global     lcd_write_data
    .type       lcd_write_data, @function

lcd_write_data:
    move.l  (4, %sp), %a0   /* data pointer */
    move.l  (8, %sp), %d0   /* length in words */
    add.l   %d0, %d0        /* words -> bytes */
    add.l   %a0, %d0        /* -> end address */
    lea.l   0xf0000002, %a1 /* LCD data port */
    
    move.l  %a0, %d1
    btst.l  #1, %d1         /* already longword aligned? */
    beq.s   .word1_end      /* yes: skip initial word copy */
    
    move.w  (%a0)+, (%a1)   /* transfer initial word */

.word1_end:                 /* now longword aligned */
    moveq.l #28, %d1
    add.l   %a0, %d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = first line bound + 16 */
    cmp.l   %d1, %d0        /* at least one full line to send? */
    blo.s   .long2_start    /* no: skip to trailing longword handling */

    lea.l   (-16, %sp), %sp /* free up some registers */
    movem.l %d2-%d4/%a2, (%sp)

    subq.l  #8, %d1
    subq.l  #8, %d1         /* %d1 = first line bound */

    cmp.l   %a0, %d1        /* any leading longwords? */
    bls.s   .long1_end      /* no: skip leading long loop */
    
.long1_loop:
    move.l  (%a0)+, %d2     /* read longword */
    swap    %d2             /* send data to LCD in correct order...*/
    move.w  %d2, (%a1)
    swap    %d2
    move.w  %d2, (%a1)
    cmp.l   %a0, %d1        /* run %a0 up to first line bound */
    bhi.s   .long1_loop
    
.long1_end:
    move.l  %d0, %a2
    lea.l   (-14, %a2), %a2 /* %a2 = end address - 14 (one line/pass) */

    /* burst-optimised line transfers */
.line_loop:
    movem.l (%a0), %d1-%d4  /* burst-read line */
    lea.l   (16, %a0), %a0  /* increment address */
    swap    %d1             /* send data to LCD in correct order... */
    move.w  %d1, (%a1)
    swap    %d1
    move.w  %d1, (%a1)
    swap    %d2
    move.w  %d2, (%a1)
    swap    %d2
    move.w  %d2, (%a1)
    swap    %d3
    move.w  %d3, (%a1)
    swap    %d3
    move.w  %d3, (%a1)
    swap    %d4
    move.w  %d4, (%a1)
    swap    %d4
    move.w  %d4, (%a1)
    cmp.l   %a0, %a2        /* run %a0 up to last line bound */
    bhi.s   .line_loop
    
    movem.l (%sp), %d2-%d4/%a2
    lea.l   (16, %sp), %sp  /* restore registers */

.long2_start:
    subq.l  #2, %d0         /* account for handling 2 words per loop */
    cmp.l   %a0, %d0        /* any (trailing longwords? */
    bls.s   .long2_end      /* no: skip trailing longword loop */

.long2_loop:
    move.l  (%a0)+, %d1     /* read longword */
    swap    %d1             /* send data to LCD in correct order */
    move.w  %d1, (%a1)
    swap    %d1
    move.w  %d1, (%a1)
    cmp.l   %a0, %d0        /* run %a0 up to last long bound */
    bhi.s   .long2_loop
    
.long2_end:
    blo.s   .word2_end      /* no final word: skip */
    move.w  (%a0)+, (%a1)   /* transfer final word */

.word2_end:
    rts
.lcd_write_data_end:
    .size   lcd_write_data, .lcd_write_data_end - lcd_write_data


/* lcd_write_yuv420_lines(), based on lcd-as-x5.S
 *
 * See http://en.wikipedia.org/wiki/YCbCr
 *   ITU-R BT.601 (formerly CCIR 601):
 *   |Y'|   | 0.299000  0.587000  0.114000| |R|
 *   |Pb| = |-0.168736 -0.331264  0.500000| |G| or 0.564334*(B - Y')
 *   |Pr|   | 0.500000 -0.418688  0.081312| |B| or 0.713267*(R - Y')
 *   Scaled, normalized and rounded:
 *   |Y'|   | 65  129   25| |R| +  16 : 16->235
 *   |Cb| = |-38  -74  112| |G| + 128 : 16->240
 *   |Cr|   |112  -94  -18| |B| + 128 : 16->240
 *
 *   The inverse:
 *   |R|   |1.000000  0.000000  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB666, as converting
 *   directly to RGB565 gives too much roundoff error:
 *   |R|   |74    0  101| |Y' -  16| / 256
 *   |G| = |74  -24  -51| |Cb - 128| / 256
 *   |B|   |74  128    0| |Cr - 128| / 256
 */

    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, @function

lcd_write_yuv420_lines:
    lea.l   (-36, %sp), %sp         /* free up some registers */
    movem.l %d2-%d6/%a2-%a5, (%sp)

    lea.l   0xf0000002, %a0         /* LCD data port */
    movem.l (36+4, %sp), %a1-%a5    /* Y data, Cb data, guv storage, Cr data, width */
    lea.l   (%a1, %a5), %a5         /* end address */

.yuv_line_loop1:
    /* chroma for first & second pixel */
    clr.l   %d1             /* load bu component */
    move.b  (%a2), %d1
    clr.l   %d3             /* load rv component */
    move.b  (%a4), %d3
    moveq.l #-128, %d0
    add.l   %d0, %d1
    add.l   %d0, %d3

    move.l  %d1, %d2        /* %d2 = cb component for guv */
    asr.l   #1, %d1         /* %d1 = 128 * (Cb - 128) / 256 */
    move.b  %d1, (%a2)+     /* save bu for next line */
    moveq.l #-24, %d0
    muls.w  %d0, %d2        /* %d2 = -24 * (Cb - 128)*/
    moveq.l #-51, %d0
    muls.w  %d3, %d0
    add.l   %d0, %d2        /* %d2 = -24 * (Cb - 128) - 51 * (Cr - 128) */
    asr.l   #8, %d2
    move.b  %d2, (%a3)+     /* save guv for next line */
    moveq.l  #101, %d0
    muls.w  %d0, %d3        /* %d3 = 101 * (Cr - 128) */
    asr.l   #8, %d3
    move.b  %d3, (%a4)+     /* save rv for next line */

    /* luma for first pixel */
    clr.l   %d4             /* load y component */
    move.b  (%a1)+, %d4
    moveq.l #74, %d0
    muls.w  %d0, %d4        /* %d4 = 36 * Y */
    asr.l   #8, %d4         
    subq.l  #4, %d4         /* correction for (Y - 16) and rounding */
    move.l  %d4, %d5
    move.l  %d4, %d6
    
    /* combine & write first pixel */
    add.l   %d1, %d4        /* %d4 = blue */
    add.l   %d2, %d5        /* %d5 = green */
    add.l   %d3, %d6        /* %d6 = red */
    
    move.l  %d4, %d0        /* clamping */
    or.l    %d5, %d0
    or.l    %d6, %d0
    asr.l   #6, %d0
    beq.s   .yuv_all_ok1
    moveq.l #63, %d0
    cmp.l   %d0, %d4
    bls.s   .yuv_blue_ok1
    spl.b   %d4
    and.l   %d0, %d4
.yuv_blue_ok1:
    cmp.l   %d0, %d5
    bls.s   .yuv_green_ok1
    spl.b   %d5
    and.l   %d0, %d5
.yuv_green_ok1:
    cmp.l   %d0, %d6
    bls.s   .yuv_red_ok1
    spl.b   %d6
    and.l   %d0, %d6
.yuv_red_ok1:
.yuv_all_ok1:

    lsr.l   #1, %d6         /* pack, convert to RGB565 and output */
    lsr.l   #1, %d4
    lsl.l   #6, %d6
    or.l    %d6, %d5
    lsl.l   #5, %d5
    or.l    %d5, %d4
    move.w  %d4, (%a0)

    /* luma for second pixel */
    clr.l   %d4             /* load y component */
    move.b  (%a1)+, %d4
    moveq.l #74, %d0
    muls.w  %d0, %d4        /* %d4 = 36 * Y */
    asr.l   #8, %d4         
    subq.l  #4, %d4         /* correction for (Y - 16) and rounding */

    /* combine & write second pixel */
    add.l   %d4, %d1        /* %d1 = blue */
    add.l   %d4, %d2        /* %d2 = green */
    add.l   %d4, %d3        /* %d3 = red */
    
    move.l  %d1, %d0        /* clamping */
    or.l    %d2, %d0
    or.l    %d3, %d0
    asr.l   #6, %d0
    beq.s   .yuv_all_ok2
    moveq.l #63, %d0
    cmp.l   %d0, %d1
    bls.s   .yuv_blue_ok2
    spl.b   %d1
    and.l   %d0, %d1
.yuv_blue_ok2:
    cmp.l   %d0, %d2
    bls.s   .yuv_green_ok2
    spl.b   %d2
    and.l   %d0, %d2
.yuv_green_ok2:
    cmp.l   %d0, %d3
    bls.s   .yuv_red_ok2
    spl.b   %d3
    and.l   %d0, %d3
.yuv_red_ok2:
.yuv_all_ok2:

    lsr.l   #1, %d3         /* pack, convert to RGB565 and output */
    lsr.l   #1, %d1
    lsl.l   #6, %d3
    or.l    %d3, %d2
    lsl.l   #5, %d2
    or.l    %d2, %d1
    move.w  %d1, (%a0)

    cmp.l   %a1,%a5         /* run %a1 up to end of line */
    bhi.w   .yuv_line_loop1

    /* Rewind chroma pointers */
    movem.l (36+8, %sp), %a2-%a5    /* bu data, guv data, rv data, width */
    lea.l   (%a1, %a5), %a5         /* next end address */

.yuv_line_loop2:
    /* read saved chromas and sign extend */
    move.b  (%a2)+, %d1
    extb.l  %d1
    move.b  (%a3)+, %d2
    extb.l  %d2
    move.b  (%a4)+, %d3
    extb.l  %d3

    /* luma for first pixel */
    clr.l   %d4             /* load y component */
    move.b  (%a1)+, %d4
    moveq.l #74, %d0
    muls.w  %d0, %d4        /* %d4 = 36 * Y */
    asr.l   #8, %d4         
    subq.l  #4, %d4         /* correction for (Y - 16) and rounding */
    move.l  %d4, %d5
    move.l  %d4, %d6
    
    /* combine & write first pixel */
    add.l   %d1, %d4        /* %d4 = blue */
    add.l   %d2, %d5        /* %d5 = green */
    add.l   %d3, %d6        /* %d6 = red */
    
    move.l  %d4, %d0        /* clamping */
    or.l    %d5, %d0
    or.l    %d6, %d0
    asr.l   #6, %d0
    beq.s   .yuv_all_ok3
    moveq.l #63, %d0
    cmp.l   %d0, %d4
    bls.s   .yuv_blue_ok3
    spl.b   %d4
    and.l   %d0, %d4
.yuv_blue_ok3:
    cmp.l   %d0, %d5
    bls.s   .yuv_green_ok3
    spl.b   %d5
    and.l   %d0, %d5
.yuv_green_ok3:
    cmp.l   %d0, %d6
    bls.s   .yuv_red_ok3
    spl.b   %d6
    and.l   %d0, %d6
.yuv_red_ok3:
.yuv_all_ok3:

    lsr.l   #1, %d6         /* pack, convert to RGB565 and output */
    lsr.l   #1, %d4
    lsl.l   #6, %d6
    or.l    %d6, %d5
    lsl.l   #5, %d5
    or.l    %d5, %d4
    move.w  %d4, (%a0)

    /* luma for second pixel */
    clr.l   %d4             /* load y component */
    move.b  (%a1)+, %d4
    moveq.l #74, %d0
    muls.w  %d0, %d4        /* %d4 = 36 * Y */
    asr.l   #8, %d4         
    subq.l  #4, %d4         /* correction for (Y - 16) and rounding */

    /* combine & write second pixel */
    add.l   %d4, %d1        /* %d1 = blue */
    add.l   %d4, %d2        /* %d2 = green */
    add.l   %d4, %d3        /* %d3 = red */
    
    move.l  %d1, %d0        /* clamping */
    or.l    %d2, %d0
    or.l    %d3, %d0
    asr.l   #6, %d0
    beq.s   .yuv_all_ok4
    moveq.l #63, %d0
    cmp.l   %d0, %d1
    bls.s   .yuv_blue_ok4
    spl.b   %d1
    and.l   %d0, %d1
.yuv_blue_ok4:
    cmp.l   %d0, %d2
    bls.s   .yuv_green_ok4
    spl.b   %d2
    and.l   %d0, %d2
.yuv_green_ok4:
    cmp.l   %d0, %d3
    bls.s   .yuv_red_ok4
    spl.b   %d3
    and.l   %d0, %d3
.yuv_red_ok4:
.yuv_all_ok4:

    lsr.l   #1, %d3         /* pack, convert to RGB565 and output */
    lsr.l   #1, %d1
    lsl.l   #6, %d3
    or.l    %d3, %d2
    lsl.l   #5, %d2
    or.l    %d2, %d1
    move.w  %d1, (%a0)

    cmp.l   %a1, %a5        /* run %a1 up to end of line */
    bhi.w   .yuv_line_loop2

    movem.l (%sp), %d2-%d6/%a2-%a5
    lea.l   (36, %sp), %sp  /* restore registers */

    rts
.lcd_write_yuv420_lines_end:
    .size   lcd_write_yuv420_lines, .lcd_write_yuv420_lines_end - lcd_write_yuv420_lines
