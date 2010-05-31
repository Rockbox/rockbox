/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
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

    .align      2
    .global     lcd_write_command
    .type       lcd_write_command,@function

lcd_write_command:
    move.l  (4, %sp), %d0
    move.w  %d0, 0xf0008000
    rts
.wc_end:
    .size   lcd_write_command,.wc_end-lcd_write_command


    .align      2
    .global     lcd_write_command_ex
    .type       lcd_write_command_ex,@function

lcd_write_command_ex:
    lea.l   0xf0008000, %a0

    move.l  (4, %sp), %d0       /* Command */
    move.w  %d0, (%a0)+         /* Write to LCD, set A0 = 1 */

    move.l  (8, %sp), %d0       /* Data */
    cmp.l   #-1, %d0            /* -1? */
    beq.b   .last
    move.w  %d0, (%a0)          /* Write to LCD */

    move.l  (12, %sp), %d0      /* Data */
    cmp.l   #-1, %d0            /* -1? */
    beq.b   .last
    move.w  %d0, (%a0)          /* Write to LCD */

.last:
    rts
.wcex_end:
    .size   lcd_write_command_ex,.wcex_end-lcd_write_command_ex


    .align      2
    .global     lcd_write_data
    .type       lcd_write_data,@function

lcd_write_data:
    movem.l (4, %sp), %a0-%a1   /* Data pointer */
    move.l  %a1, %d0            /* Length */
    lea     0xf0008002, %a1

.loop:
    /* When running in IRAM, this loop takes 10 cycles plus the LCD write.
       The 10 cycles are necessary to follow the LCD timing specs
       at 140MHz */
    nop                         /* 3(0/0) */
    move.b  (%a0)+, %d1         /* 3(1/0) */
    move.w  %d1, (%a1)          /* 1(0/1) */
    subq.l  #1, %d0             /* 1(0/0) */
    bne     .loop               /* 2(0/0) */
    rts
.wd_end:
    .size   lcd_write_data,.wd_end-lcd_write_data


    .align      2
    .global     lcd_grey_data
    .type       lcd_grey_data,@function

    /* The main loop assumes the buffers are in SDRAM. Otherwise the LCD
     * controller timing won't be met at 124 MHz and graphical glitches 
     * will occur. */

lcd_grey_data:
    lea.l   (-11*4, %sp), %sp
    movem.l %d2-%d7/%a2-%a6, (%sp)  /* free some registers */
    movem.l (11*4+4, %sp), %a0-%a2  /* values, phases, length */
    lea.l   (%a1, %a2.l*4), %a2     /* end address */
    lea     0xf0008002, %a3     /* LCD data port */
    moveq.l #24, %d6            /* shift count */
    move.l  #0xc30c3, %d7       /* bit shuffle factor */

    moveq.l #12, %d2
    add.l   %a1, %d2
    and.l   #0xfffffff0, %d2    /* first line bound */
    cmp.l   %d2, %a2            /* end address lower than first line bound? */
    bhs.s   1f
    move.l  %a2, %d2            /* -> adjust end address of head loop */
1:
    cmp.l   %a1, %d2
    bls.s   .g_hend

.g_hloop:
    move.l  (%a1), %d0          /* fetch 4 pixel phases */

    move.l  %d0, %d1
    and.l   #0x80808080, %d1    /* separate MSBs of the 4 phases */
    eor.l   %d1, %d0            /* clear them in %d0 */
    add.l   (%a0)+, %d0         /* add 4 pixel values to the phases */
    move.l  %d0, (%a1)+         /* store new phases, advance pointer */

    lsr.l   #1, %d1             /* %d1 = .0.......1.......2.......3...... */
    mulu.l  %d7, %d1            /* %d1 = 00112233112233..2233....33...... */
    not.l   %d1                 /*       negate bits */
    lsr.l   %d6, %d1            /* %d1 = ........................00112233 */
    move.w  %d1, (%a3)          /* write pixel block */

    cmp.l   %a1, %d2            /* go up to first line bound */
    bhi.s   .g_hloop

.g_hend:
    cmp.l   %a1, %a2
    bls.w   .g_tend
    lea.l   (-12, %a2), %a2
    cmp.l   %a1, %a2
    bls.s   .g_lend
    
.g_lloop:
    movem.l (%a1), %d0-%d3      /* fetch 4 blocks of 4 pixel phases each */

    move.l  %d0, %d4            /* calculate first pixel block */
    and.l   #0x80808080, %d4
    eor.l   %d4, %d0            
    lsr.l   #1, %d4             
    mulu.l  %d7, %d4            
    not.l   %d4
    lsr.l   %d6, %d4

    move.w  %d4, (%a3)          /* write first pixel block to LCD */

    move.l  %d1, %d5            /* calculate second pixel block */
    and.l   #0x80808080, %d5
    eor.l   %d5, %d1
    lsr.l   #1, %d5
    mulu.l  %d7, %d5
    not.l   %d5
    lsr.l   %d6, %d5
                                
    move.l  %d2, %d4            /* calculate third pixel block */
    and.l   #0x80808080, %d4    
    eor.l   %d4, %d2            
    lsr.l   #1, %d4
    mulu.l  %d7, %d4            
    not.l   %d4
    lsr.l   %d6, %d4

    move.w  %d5, (%a3)          /* write second pixel block to LCD */

    movem.l (%a0), %d5/%a4-%a6  /* fetch 4 blocks of 4 pixel values each */
    lea.l   (16, %a0), %a0

    move.w  %d4, (%a3)          /* write third pixel block to LCD */
                                
    move.l  %d3, %d4            /* calculate fourth pixel block */
    and.l   #0x80808080, %d4
    eor.l   %d4, %d3
    lsr.l   #1, %d4
    mulu.l  %d7, %d4            
    not.l   %d4
    lsr.l   %d6, %d4

    add.l   %d5, %d0            /* calculate 4*4 new pixel phases */
    add.l   %a4, %d1            /* (packed addition) */
    add.l   %a5, %d2
    add.l   %a6, %d3

    movem.l %d0-%d3, (%a1)      /* store 4*4 new pixel phases */
    lea.l   (16, %a1), %a1

    move.w  %d4, (%a3)          /* write fourth pixel block to LCD */

    cmp.l   %a1, %a2            /* go up to last line bound */
    bhi.s   .g_lloop
    
.g_lend:
    lea.l   (12, %a2), %a2
    cmp.l   %a1, %a2
    bls.s   .g_tend

.g_tloop:
    move.l  (%a1), %d0          /* fetch 4 pixel phases */

    move.l  %d0, %d1
    and.l   #0x80808080, %d1
    eor.l   %d1, %d0            
    add.l   (%a0)+, %d0         /* add 4 pixel values to the phases */
    move.l  %d0, (%a1)+         /* store new phases, advance pointer */

    lsr.l   #1, %d1             
    mulu.l  %d7, %d1
    not.l   %d1
    lsr.l   %d6, %d1
    move.w  %d1, (%a3)          /* write pixel block */

    cmp.l   %a1, %a2            /* go up to end address */
    bhi.s   .g_tloop

.g_tend:
    movem.l (%sp), %d2-%d7/%a2-%a6  /* restore registers */
    lea.l   (11*4, %sp), %sp
    rts

.gd_end:
    .size   lcd_grey_data,.gd_end-lcd_grey_data
