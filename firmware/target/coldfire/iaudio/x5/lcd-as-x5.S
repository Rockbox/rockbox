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
