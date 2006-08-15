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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
 *   |R|   |74    0  102| |Y' -  16| / 256
 *   |G| = |74  -25  -52| |Cb - 128| / 256
 *   |B|   |74  129    0| |Cr - 128| / 256
 */
    .align      2
    .global     lcd_write_yuv420_lines
    .type       lcd_write_yuv420_lines,@function
lcd_write_yuv420_lines:
    lea.l   (-40,%sp),%sp   /* free up some registers */
    movem.l %d2-%d7/%a2-%a5,(%sp)

    lea.l   0xf0008002,%a0  /* LCD data port */
    move.l  (40+4,%sp),%a1  /* Y data */
    move.l  (40+8,%sp),%a2  /* Cb data */
    move.l  (40+12,%sp),%a3 /* guv storage */
    move.l  (40+16,%sp),%a4 /* Cr data */
    move.l  (40+20,%sp),%d0 /* width */
    lea.l   (%a1,%d0.l),%a5 /* end address */

.yuv_line_loop1:
    /** Write first pixel **/
    clr.l   %d1             /* get y component */
    move.b  (%a1)+,%d1
    subq.l  #8,%d1
    subq.l  #8,%d1
    moveq.l #74,%d6
    muls.w  %d6,%d1
    asr.l   #8,%d1

    clr     %d2             /* get bu component */
    move.b  (%a2),%d2
    moveq.l #-128,%d6
    add.l   %d6,%d2
    move.l  %d2,%d3         /* %d3 = cb component for guv */
    move.w  #129,%d6
    muls.w  %d6,%d2
    asr.l   #8,%d2
    move.b  %d2,(%a2)+      /* save bu for next line */

    moveq.l #-25,%d6        /* multiply first term of guv */
    muls.w  %d6,%d3

    clr     %d4             /* get rv component */
    move.b  (%a4),%d4
    moveq.l #-128,%d6
    add.l   %d6,%d4
    move.l  %d4,%d7         /* %d7 = cr component for guv */
    moveq.l  #102,%d6
    muls.w  %d6,%d4
    asr.l   #8,%d4
    move.b  %d4,(%a4)+      /* save rv for next line */

    moveq.l #-52,%d6        /* multiply second term of guv */
    muls.w  %d6,%d7
    add.l   %d7,%d3
    asr.l   #8,%d3
    move.b  %d3,(%a3)+      /* save guv for next line */
    /* : %d1 = Y, %d2 = bu, %d3 = guv, %d4 = rv */

    move.l  %d1,%d5         /* get r */
    add.l   %d4,%d5
    move.l  %d1,%d6         /* get g */
    add.l   %d3,%d6
    move.l  %d1,%d7         /* get b */
    add.l   %d2,%d7

    move.l  %d7,%d1         /* is clamping needed? */
    or.l    %d6,%d1
    or.l    %d5,%d1
    asr.l   #6,%d1
    beq.b   .yuv_no_clamp1  /* values in range: skip clamping */
    bpl.b   .yuv_r63_test1  /* no negative values: skip to high bounds checks */
.yuv_r0_test1:
    clr.l   %d1             /* check for any values < 0 */
    cmp.l   %d1,%d5
    bgt.b   .yuv_g0_test1
    clr.l   %d5
.yuv_g0_test1:
    cmp.l   %d1,%d6
    bgt.b   .yuv_b0_test1
    clr.l   %d6
.yuv_b0_test1:
    cmp.l   %d1,%d7
    bgt.b   .yuv_r63_test1
    clr.l   %d7
.yuv_r63_test1:             /* check for any values > 63 */
    moveq.l #63,%d1
    cmp.l   %d1,%d5
    blt.b   .yuv_g63_test1
    move.l  %d1,%d5
.yuv_g63_test1:
    cmp.l   %d1,%d6
    blt.b   .yuv_b63_test1
    move.l  %d1,%d6
.yuv_b63_test1:
    cmp.l   %d1,%d7
    blt.b   .yuv_no_clamp1
    move.l  %d1,%d7
.yuv_no_clamp1:
    /* : %d5 = R, %d6 = G, %d7 = B */

    move.l  %d6,%d1         /* save g for lower 9 bits */
    lsl.l   #3,%d5          /* R << 3 */
    lsr.l   #3,%d1          /* G >> 3 */
    or.l    %d5,%d1
    move.w  %d1,(%a0)       /* |00000000|000000000|0000000r|rrrrrggg| */
    lsl.l   #6,%d6          /* B << 6 */
    or.l    %d6,%d7         /* |00000000|000000000|0000gggg|ggbbbbbb| */
    move.w  %d7,(%a0)

    /** Write second pixel **/
    clr     %d1
    move.b  (%a1)+,%d1      /* get y component */
    subq.l  #8,%d1
    subq.l  #8,%d1
    moveq.l #74,%d6
    muls.w  %d6,%d1
    asr.l   #8,%d1
    /* : %d1 = Y, %d2 = bu, %d3 = guv, %d4 = rv */

    /* Add Y + each chroma component (can clobber %d2-%d4 values now) */
    add.l   %d1,%d4         /* get r */
    add.l   %d1,%d3         /* get g */
    add.l   %d1,%d2         /* get b */

    move.l  %d2,%d1         /* is clamping needed? */
    or.l    %d3,%d1
    or.l    %d4,%d1
    asr.l   #6,%d1
    beq.b   .yuv_no_clamp2  /* values in range: skip clamping */
    bpl.b   .yuv_r63_test2  /* no negative values: skip to high bounds checks */
.yuv_r0_test2:
    clr.l   %d1             /* check for any values < 0 */
    cmp.l   %d1,%d4
    bgt.b   .yuv_g0_test2
    clr.l   %d4
.yuv_g0_test2:
    cmp.l   %d1,%d3
    bgt.b   .yuv_b0_test2
    clr.l   %d3
.yuv_b0_test2:
    cmp.l   %d1,%d2
    bgt.b   .yuv_r63_test2
    clr.l   %d2
.yuv_r63_test2:             /* check for any values > 63 */
    moveq.l #63,%d1
    cmp.l   %d1,%d4
    blt.b   .yuv_g63_test2
    move.l  %d1,%d4
.yuv_g63_test2:
    cmp.l   %d1,%d3
    blt.b   .yuv_b63_test2
    move.l  %d1,%d3
.yuv_b63_test2:
    cmp.l   %d1,%d2
    blt.b   .yuv_no_clamp2
    move.l  %d1,%d2
.yuv_no_clamp2:
    /* : %d4 = R, %d3 = G, %d2 = B */

    move.l  %d3,%d1         /* save g for lower 9 bits */
    lsl.l   #3,%d4          /* R << 3 */
    lsr.l   #3,%d1          /* G >> 3 */
    or.l    %d4,%d1         /* |00000000|000000000|0000000r|rrrrrggg| */
    move.w  %d1,(%a0)
    lsl.l   #6,%d3          /* G << 6 */
    or.l    %d3,%d2         /* |00000000|000000000|0000gggg|ggbbbbbb| */
    move.w  %d2,(%a0)

    cmp.l   %a1,%a5         /* run %a1 up to end of line */
    bhi.w   .yuv_line_loop1

    /* Rewind chroma pointers */
    move.l  (40+8,%sp),%a2  /* bu data */
    move.l  (40+12,%sp),%a3 /* guv data */
    move.l  (40+16,%sp),%a4 /* rv data */
    lea.l   (%a5,%d0),%a5   /* next end address */

.yuv_line_loop2:
    clr     %d1
    move.b  (%a1)+,%d1      /* get y component */
    subq.l  #8,%d1
    subq.l  #8,%d1
    moveq.l #74,%d6
    muls.w  %d6,%d1
    asr.l   #8,%d1

    move.b  (%a2)+,%d2      /* read save chromas and sign extend */
    extb.l  %d2
    move.b  (%a3)+,%d3
    extb.l  %d3
    move.b  (%a4)+,%d4
    extb.l  %d4
    /* : %d1 = Y, %d2 = bu, %d3 = guv, %d4 = rv */

    move.l  %d1,%d5         /* get r */
    add.l   %d4,%d5
    move.l  %d1,%d6         /* get g */
    add.l   %d3,%d6
    move.l  %d1,%d7         /* get b */
    add.l   %d2,%d7

    move.l  %d7,%d1         /* is clamping needed? */
    or.l    %d6,%d1
    or.l    %d5,%d1
    asr.l   #6,%d1
    beq.b   .yuv_no_clamp3  /* values in range: skip clamping */
    bpl.b   .yuv_r63_test3  /* no negative values: skip to high bounds checks */
.yuv_r0_test3:
    clr.l   %d1             /* check for any values < 0 */
    cmp.l   %d1,%d5
    bgt.b   .yuv_g0_test3
    clr.l   %d5
.yuv_g0_test3:
    cmp.l   %d1,%d6
    bgt.b   .yuv_b0_test3
    clr.l   %d6
.yuv_b0_test3:
    cmp.l   %d1,%d7
    bgt.b   .yuv_r63_test3
    clr.l   %d7
.yuv_r63_test3:             /* check for any values > 63 */
    moveq.l #63,%d1
    cmp.l   %d1,%d5
    blt.b   .yuv_g63_test3
    move.l  %d1,%d5
.yuv_g63_test3:
    cmp.l   %d1,%d6
    blt.b   .yuv_b63_test3
    move.l  %d1,%d6
.yuv_b63_test3:
    cmp.l   %d1,%d7
    blt.b   .yuv_no_clamp3
    move.l  %d1,%d7
.yuv_no_clamp3:
    /* : %d5 = R, %d6 = G, %d7 = B */

    move.l  %d6,%d1         /* save g for lower 9 bits */
    lsl.l   #3,%d5          /* R << 3 */
    lsr.l   #3,%d1          /* G >> 3 */
    or.l    %d5,%d1
    move.w  %d1,(%a0)       /* |00000000|000000000|0000000r|rrrrrggg| */
    lsl.l   #6,%d6          /* B << 6 */
    or.l    %d6,%d7         /* |00000000|000000000|0000gggg|ggbbbbbb| */
    move.w  %d7,(%a0)

    /** Write second pixel **/
    clr     %d1
    move.b  (%a1)+,%d1      /* get y component */
    subq.l  #8,%d1
    subq.l  #8,%d1
    moveq.l #74,%d6
    muls.w  %d6,%d1
    asr.l   #8,%d1
    /* : %d1 = Y, %d2 = bu, %d3 = guv, %d4 = rv */

    /* Add Y + each chroma component (can clobber %d2-%d4 values now) */
    add.l   %d1,%d4         /* get r */
    add.l   %d1,%d3         /* get g */
    add.l   %d1,%d2         /* get b */

    move.l  %d2,%d1         /* is clamping needed? */
    or.l    %d3,%d1
    or.l    %d4,%d1
    asr.l   #6,%d1
    beq.b   .yuv_no_clamp4  /* values in range: skip clamping */
    bpl.b   .yuv_r63_test4  /* no negative values: skip to high bounds checks */
.yuv_r0_test4:
    clr.l   %d1             /* check for any values < 0 */
    cmp.l   %d1,%d4
    bgt.b   .yuv_g0_test4
    clr.l   %d4
.yuv_g0_test4:
    cmp.l   %d1,%d3
    bgt.b   .yuv_b0_test4
    clr.l   %d3
.yuv_b0_test4:
    cmp.l   %d1,%d2
    bgt.b   .yuv_r63_test4
    clr.l   %d2
.yuv_r63_test4:             /* check for any values > 63 */
    moveq.l #63,%d1
    cmp.l   %d1,%d4
    blt.b   .yuv_g63_test4
    move.l  %d1,%d4
.yuv_g63_test4:
    cmp.l   %d1,%d3
    blt.b   .yuv_b63_test4
    move.l  %d1,%d3
.yuv_b63_test4:
    cmp.l   %d1,%d2
    blt.b   .yuv_no_clamp4
    move.l  %d1,%d2
.yuv_no_clamp4:
    /* : %d4 = R, %d3 = G, %d2 = B */

    move.l  %d3,%d1         /* save g for lower 9 bits */
    lsl.l   #3,%d4          /* R << 3 */
    lsr.l   #3,%d1          /* G >> 3 */
    or.l    %d4,%d1         /* |00000000|000000000|0000000r|rrrrrggg| */
    move.w  %d1,(%a0)
    lsl.l   #6,%d3          /* G << 6 */
    or.l    %d3,%d2         /* |00000000|000000000|0000gggg|ggbbbbbb| */
    move.w  %d2,(%a0)

    cmp.l   %a1,%a5         /* run %a0 up to end of line */
    bhi.w   .yuv_line_loop2

    movem.l (%sp),%d2-%d7/%a2-%a5
    lea.l   (40,%sp),%sp    /* restore registers */

    rts
/* end lcd_write_yuv420_lines */


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

    lea.l   (-24,%sp),%sp   /* free up some registers */
    movem.l %d2-%d6/%a2,(%sp)

    move.l  %a0,%d1
    btst.l  #1,%d1          /* already longword aligned? */
    beq.b   .wd_wordl_end   /* yes: skip initial word copy */
                            /* transfer initial word */
    move.w  (%a0)+,%d1      /* |????????|????????|rrrrrggg|gggbbbbb| */
    move.l  %d1,%d2
    and.l   #0xfffff800,%d2 /* |????????|????????|rrrrr000|00000000| */
    add.l   %d2,%d1         /* |????????|???????r|rrrr0ggg|gggbbbbb| */
    move.l  %d1,%d2
    lsr.l   #8,%d1          /* |00000000|????????|???????r|rrrr0ggg| */
    move.w  %d1,(%a1)
    lsl.l   #1,%d2          /* |????????|??????rr|rrr0gggg|ggbbbbb0| */
    move.w  %d2,(%a1)

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
    swap    %d2             /* unstuff two pixels and correct order */
    move.l  %d2,%d5
    and.l   #0xff00ff00,%d5 /* |rrrrrggg|00000000|rrrrrggg|00000000| */
    eor.l   %d5,%d2         /* |00000000|gggbbbbb|00000000|gggbbbbb| */
    lsr.l   #8,%d5          /* |00000000|rrrrrggg|00000000|rrrrrggg| */
    move.l  %d5,%d6
    and.l   #0x00f800f8,%d5 /* |00000000|rrrrr000|00000000|rrrrr000| */
    add.l   %d6,%d5         /* |0000000r|rrrr0ggg|0000000r|rrrr0ggg| */
    move.w  %d5,(%a1)
    lsl.l   #1,%d2          /* |0000000g|ggbbbbb0|0000000g|ggbbbbb0| */
    move.w  %d2,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d2
    move.w  %d2,(%a1)
    cmp.l   %a0,%d1
    bhi.b   .wd_long1_loop

.wd_long1_end:
    move.l  %d0,%a2
    lea.l   (-14,%a2),%a2

.wd_line_loop:
    movem.l (%a0),%d1-%d4   /* burst-read eight words */
    lea.l   (16,%a0),%a0    /* increment address */

    /* transfer four pairs of longs to display */
    /* same procedure for each as in leading long loop */
    swap    %d1
    move.l  %d1,%d5
    and.l   #0xff00ff00,%d5
    eor.l   %d5,%d1
    lsr.l   #8,%d5
    move.l  %d5,%d6
    and.l   #0x00f800f8,%d5
    add.l   %d6,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d1
    move.w  %d1,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d1
    move.w  %d1,(%a1)

    swap    %d2
    move.l  %d2,%d5
    and.l   #0xff00ff00,%d5
    eor.l   %d5,%d2
    lsr.l   #8,%d5
    move.l  %d5,%d6
    and.l   #0x00f800f8,%d5
    add.l   %d6,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d2
    move.w  %d2,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d2
    move.w  %d2,(%a1)

    swap    %d3
    move.l  %d3,%d5
    and.l   #0xff00ff00,%d5
    eor.l   %d5,%d3
    lsr.l   #8,%d5
    move.l  %d5,%d6
    and.l   #0x00f800f8,%d5
    add.l   %d6,%d5
    move.w  %d5,(%a1)
    lsl.l   #1,%d3
    move.w  %d3,(%a1)
    swap    %d5
    move.w  %d5,(%a1)
    swap    %d3
    move.w  %d3,(%a1)

    swap    %d4
    move.l  %d4,%d5
    and.l   #0xff00ff00,%d5
    eor.l   %d5,%d4
    lsr.l   #8,%d5
    move.l  %d5,%d6
    and.l   #0x00f800f8,%d5
    add.l   %d6,%d5
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
    and.l   #0xff00ff00,%d5
    eor.l   %d5,%d2
    lsr.l   #8,%d5
    move.l  %d5,%d6
    and.l   #0x00f800f8,%d5
    add.l   %d6,%d5
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

    move.w  (%a0),%d1       /* transfer final word */
    move.l  %d1,%d2
    and.l   #0xfffff800,%d2
    add.l   %d2,%d1
    move.l  %d1,%d2
    lsr.l   #8,%d1
    move.w  %d1,(%a1)
    lsl.l   #1,%d2
    move.w  %d2,(%a1)

.wd_word2_end:
    movem.l (%sp),%d2-%d6/%a2
    lea.l   (24,%sp),%sp    /* restore registers */
    rts
/* end lcd_write_data */
