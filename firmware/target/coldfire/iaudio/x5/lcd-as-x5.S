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
