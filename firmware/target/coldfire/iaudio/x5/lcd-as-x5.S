/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
 * Based on lcd_write_data for H300 in lcd.S
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

    .section    .icode,"ax",@progbits

/* begin lcd_write_yuv420_lines
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
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB 666:
 *   |R|   |19611723        0  26881894| |Y' -  16| >> 26
 *   |G| = |19611723 -6406711 -13692816| |Cb - 128| >> 26
 *   |B|   |19611723 33976259         0| |Cr - 128| >> 26
 *
 * Needs EMAC set to saturated, signed integer mode.
 *
 * register usage:
 *   %a0 - LCD data port
 *   %a1 - Y pointer
 *   %a2 - C pointer
 *   %a3 - C width
 *   %a4 - Y end address
 *   %a5 - Y factor
 *   %a6 - BU factor
 *   %d0 - scratch
 *   %d1 -     B, previous Y \ alternating
 *   %d2 - U / B, previous Y /
 *   %d3 - V / G
 *   %d4 - R / output pixel
 *   %d5 - GU factor
 *   %d6 - GV factor
 *   %d7 - RGB signed -> unsigned conversion mask
 */
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines, @function

lcd_write_yuv420_lines:
    lea.l   (-44, %sp), %sp         /* free up some registers */
    movem.l %d2-%d7/%a2-%a6, (%sp)

    lea.l   0xf0008002, %a0         /* LCD data port */
    movem.l (44+4, %sp), %a1-%a3    /* Y data, C data, C width */
    lea.l   (%a1, %a3*2), %a4       /* Y end address */

    move.l  #19611723, %a5          /* y factor */
    move.l  #33976259, %a6          /* bu factor */
    move.l  #-6406711, %d5          /* gu factor */
    move.l  #-13692816, %d6         /* gv factor */
    move.l  #0x01040820, %d7        /* bitmask for signed->unsigned conversion
                                     * of R, G and B within RGGB6666 at once */

    /* chroma for first 2x2 block */
    clr.l   %d3                     /* load v component */
    move.b  (%a2, %a3), %d3
    clr.l   %d2                     /* load u component */
    move.b  (%a2)+, %d2
    moveq.l #-128, %d0
    add.l   %d0, %d2
    add.l   %d0, %d3

    mac.l   %a6, %d2, %acc0         /* bu */
    mac.l   %d5, %d2, %acc1         /* gu */
    mac.l   %d6, %d3, %acc1         /* gv */
    move.l  #26881894, %d0          /* rv factor */
    mac.l   %d0, %d3, %acc2         /* rv */

    /* luma for very first pixel (top left) */
    clr.l   %d1
    move.b  (%a1, %a3*2), %d1
    moveq.l #-126, %d0
    add.l   %d1, %d0                /* y' (-0.5 ... +0.5) */
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    bra.b   .yuv_line_entry

.yuv_line_loop:
    /* chroma for 2x2 pixel block */
    clr.l   %d3                     /* load v component */
    move.b  (%a2, %a3), %d3
    clr.l   %d2                     /* load u component */
    move.b  (%a2)+, %d2
    moveq.l #-128, %d0
    add.l   %d0, %d2
    add.l   %d0, %d3

    mac.l   %a6, %d2, %acc0         /* bu */
    mac.l   %d5, %d2, %acc1         /* gu */
    mac.l   %d6, %d3, %acc1         /* gv */
    move.l  #26881894, %d0          /* rv factor */
    mac.l   %d0, %d3, %acc2         /* rv */

    /* luma for first pixel (top left) */
    clr.l   %d1
    move.b  (%a1, %a3*2), %d1
    moveq.l #-126, %d0
    add.l   %d1, %d0                /* y' (-0.5 ... +0.5) */
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2
    
    move.w  %d4, (%a0)
    /* 2nd LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB666, pack and output */
.yuv_line_entry:
    moveq.l #26, %d0
    move.l  %acc0, %d4
    move.l  %acc1, %d3
    move.l  %acc2, %d2
    lsr.l   %d0, %d4
    lsr.l   %d0, %d3
    lsr.l   %d0, %d2
    
    lsl.l   #6, %d2
    or.l    %d3, %d2                /* |00000000|00000000|0000Rrrr|rrGggggg| */
    lsl.l   #7, %d2
    or.l    %d2, %d3                /* |00000000|00000Rrr|rrrGgggg|g0Gggggg| */
    lsl.l   #6, %d3
    or.l    %d3, %d4                /* |0000000R|rrrrrGgg|ggg0Gggg|ggBbbbbb| */
    eor.l   %d7, %d4                /* |0000000r|rrrrrggg|ggg0gggg|ggbbbbbb| */
    swap    %d4
    move.w  %d4, (%a0)
    swap    %d4

    /* luma for second pixel (bottom left) as delta from the first */
    clr.l   %d2
    move.b  (%a1)+, %d2
    move.l  %d2, %d0
    sub.l   %d1, %d0
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    move.w  %d4, (%a0)
    /* 2nd LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB666, pack and output */
    moveq.l #26, %d0
    move.l  %acc0, %d4
    move.l  %acc1, %d3
    move.l  %acc2, %d1
    lsr.l   %d0, %d4
    lsr.l   %d0, %d3
    lsr.l   %d0, %d1

    lsl.l   #6, %d1
    or.l    %d3, %d1                /* |00000000|00000000|0000Rrrr|rrGggggg| */
    lsl.l   #7, %d1
    or.l    %d1, %d3                /* |00000000|00000Rrr|rrrGgggg|g0Gggggg| */
    lsl.l   #6, %d3
    or.l    %d3, %d4                /* |0000000R|rrrrrGgg|ggg0Gggg|ggBbbbbb| */
    eor.l   %d7, %d4                /* |0000000r|rrrrrggg|ggg0gggg|ggbbbbbb| */
    swap    %d4
    move.w  %d4, (%a0)
    swap    %d4
    
    /* luma for third pixel (top right) as delta from the second */
    clr.l   %d1
    move.b  (%a1, %a3*2), %d1
    move.l  %d1, %d0
    sub.l   %d2, %d0
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2
    
    move.w  %d4, (%a0)
    /* 2nd LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB666, pack and output */
    moveq.l #26, %d0
    move.l  %acc0, %d4
    move.l  %acc1, %d3
    move.l  %acc2, %d2
    lsr.l   %d0, %d4
    lsr.l   %d0, %d3
    lsr.l   %d0, %d2
    
    lsl.l   #6, %d2
    or.l    %d3, %d2                /* |00000000|00000000|0000Rrrr|rrGggggg| */
    lsl.l   #7, %d2
    or.l    %d2, %d3                /* |00000000|00000Rrr|rrrGgggg|g0Gggggg| */
    lsl.l   #6, %d3
    or.l    %d3, %d4                /* |0000000R|rrrrrGgg|ggg0Gggg|ggBbbbbb| */
    eor.l   %d7, %d4                /* |0000000r|rrrrrggg|ggg0gggg|ggbbbbbb| */
    swap    %d4
    move.w  %d4, (%a0)
    swap    %d4

    /* luma for fourth pixel (bottom right) as delta from the thrid */
    clr.l   %d2
    move.b  (%a1)+, %d2
    move.l  %d2, %d0
    sub.l   %d1, %d0
    mac.l   %a5, %d0, %acc0
    mac.l   %a5, %d0, %acc1
    mac.l   %a5, %d0, %acc2

    move.w  %d4, (%a0)
    /* 2nd LCD write is delayed one pixel to use it for filling the EMAC latency */

    /* convert to RGB666, pack and output */
    moveq.l #26, %d0
    movclr.l %acc0, %d4
    movclr.l %acc1, %d3
    movclr.l %acc2, %d1
    lsr.l   %d0, %d4
    lsr.l   %d0, %d3
    lsr.l   %d0, %d1

    lsl.l   #6, %d1
    or.l    %d3, %d1                /* |00000000|00000000|0000Rrrr|rrGggggg| */
    lsl.l   #7, %d1
    or.l    %d1, %d3                /* |00000000|00000Rrr|rrrGgggg|g0Gggggg| */
    lsl.l   #6, %d3
    or.l    %d3, %d4                /* |0000000R|rrrrrGgg|ggg0Gggg|ggBbbbbb| */
    eor.l   %d7, %d4                /* |0000000r|rrrrrggg|ggg0gggg|ggbbbbbb| */
    swap    %d4
    move.w  %d4, (%a0)
    swap    %d4

    cmp.l   %a1, %a4                /* run %a1 up to end of line */
    bhi.w   .yuv_line_loop

    move.w  %d4, (%a0)              /* write (very) last 2nd word */

    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l   (44, %sp), %sp  /* restore registers */
    rts
.yuv_end:
    .size   lcd_write_yuv420_lines, yuv_end - lcd_write_yuv420_lines


/* begin lcd_write_data */
    .align      2
    .global     lcd_write_data
    .type       lcd_write_data,@function
lcd_write_data:
    move.l  (4,%sp),%a0     /* data pointer */
    move.l  (8,%sp),%d0     /* length in words */
    add.l   %d0,%d0         /* words -> bytes */
    add.l   %a0,%d0         /* -> end address */
    lea.l   0xf0008002,%a1  /* LCD data port */

    lea.l   (-20,%sp),%sp   /* free up some registers */
    movem.l %d2-%d5/%a2,(%sp)

    move.l  %a0,%d1
    btst.l  #1,%d1          /* already longword aligned? */
    beq.b   .wd_wordl_end   /* yes: skip initial word copy */
                            /* transfer initial word */
    move.w  (%a0)+,%d2      /* |????????|????????|rrrrrggg|gggbbbbb| */
    move.l  %d2,%d1
    lsr.l   #7,%d1          /* |0000000?|????????|???????r|rrrrgggg| */
    move.w  %d1,(%a1)       /*                           ^ ^^^^^^^   */
    lsl.l   #1,%d2          /* |????????|???????r|rrrrgggg|ggbbbbb0| */
    move.w  %d2,(%a1)       /*                           ^ ^^^^^^^   */

.wd_wordl_end:              /* now longword aligned */
    moveq.l #28,%d1
    add.l   %a0,%d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = second line bound */
    cmp.l   %d1,%d0         /* at least one full line to send? */
    blo.w   .wd_long2_start /* no: skip to trailing longword handling */

    subq.l  #8,%d1
    subq.l  #8,%d1          /* %d1 = first line bound */

    cmp.l   %a0,%d1         /* any leading longwords? */
    bls.b   .wd_long1_end   /* no: skip leading long loop */

.wd_long1_loop:
    move.l  (%a0)+,%d2      /* read longword */
    swap    %d2             /* |rrrrrggg|gggbbbbb|RRRRRGGG|GGGBBBBB| */
    move.l  %d2,%d5
    lsr.l   #7,%d5          /* |0000000r|rrrrgggg|ggbbbbbR|RRRRGGGG| */
    move.w  %d5,(%a1)       /*                           ^ ^^^^^^^   */
    lsl.l   #1,%d2          /* |rrrrgggg|ggbbbbbR|RRRRGGGG|GGBBBBB0| */
    move.w  %d2,(%a1)       /*                           ^ ^^^^^^^   */
    swap    %d5             /* |ggbbbbbR|RRRRGGGG|0000000r|rrrrgggg| */
    move.w  %d5,(%a1)       /*                           ^ ^^^^^^^   */
    swap    %d2             /* |RRRRBGGG|GGBBBBB0|rrrrgggg|ggbbbbbR| */
    move.w  %d2,(%a1)       /*                           ^ ^^^^^^^   */
    cmp.l   %a0,%d1
    bhi.b   .wd_long1_loop

.wd_long1_end:
    move.l  %d0,%a2
    lea.l   (-14,%a2),%a2

.wd_line_loop:
    movem.l (%a0),%d1-%d4   /* burst-read eight words */
    lea.l   (16,%a0),%a0    /* increment address */

    /* transfer four pairs of longs to display */
    swap    %d1
    move.l  %d1,%d5
    lsr.l   #7,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d1
    move.w  %d1,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d1
    move.w  %d1,(%a1)

    swap    %d2
    move.l  %d2,%d5
    lsr.l   #7,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d2
    move.w  %d2,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d2
    move.w  %d2,(%a1)

    swap    %d3
    move.l  %d3,%d5
    lsr.l   #7,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d3
    move.w  %d3,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d3
    move.w  %d3,(%a1)

    swap    %d4
    move.l  %d4,%d5
    lsr.l   #7,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d4
    move.w  %d4,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d4
    move.w  %d4,(%a1)

    cmp.l   %a0,%a2         /* run %a0 up to last line bound */
    bhi.w   .wd_line_loop

.wd_long2_start:
    subq.l  #2,%d0          /* account for handling 2 words per loop */
    cmp.l   %a0,%d0         /* any (trailing longwords? */
    bls.b   .wd_long2_end   /* no: skip trailing longword loop */

.wd_long2_loop:
    move.l  (%a0)+,%d2      /* read longword */
    swap    %d2
    move.l  %d2,%d5
    lsr.l   #7,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d2
    move.w  %d2,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d2
    move.w  %d2,(%a1)
    cmp.l   %a0,%d0         /* run %a0 up to last long bound */
    bhi.b   .wd_long2_loop

.wd_long2_end:
    blo.b   .wd_word2_end   /* no final word: skip */

    move.w  (%a0)+,%d2      /* transfer final word */
    move.l  %d2,%d1
    lsr.l   #7,%d1
    move.w  %d1,(%a1)
    lsl.l   #1,%d2
    move.w  %d2,(%a1)

.wd_word2_end:
    movem.l (%sp),%d2-%d5/%a2
    lea.l   (20,%sp),%sp    /* restore registers */
    rts

.wd_end:
    .size   lcd_write_data,.wd_end-lcd_write_data
/* end lcd_write_data */
