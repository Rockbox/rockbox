/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Marcin Bukat
 * based on lcd-as-m3.S by Jens Arnold
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


#define LCD_BASE_ADDRESS 0xf0000000

    .section    .icode,"ax",@progbits

    .align      2
    .global     lcd_write_command
    .type       lcd_write_command,@function

lcd_write_command:
    move.l  (4, %sp), %d0
    move.b  %d0, LCD_BASE_ADDRESS	/* transfers 1 byte to LCD */

    rts
.wc_end:
    .size   lcd_write_command,.wc_end-lcd_write_command


    .align      2
    .global     lcd_write_command_e
    .type       lcd_write_command_e,@function

lcd_write_command_e:
    lea.l   LCD_BASE_ADDRESS, %a0
    move.l  (4, %sp), %d0       /* Command */
    move.b  %d0, (%a0)         
    move.l  (8, %sp), %d0       /* Data */
    move.b  %d0, (%a0)          /* Write to LCD */

    rts
.wce_end:
    .size   lcd_write_command_e,.wce_end-lcd_write_command_e


    .align      2
    .global     lcd_write_data
    .type       lcd_write_data,@function

/* PIXELFORMAT = VERTICAL_INTERLEAVED
 * this means that data is packed verticaly in 8 pixels columns
 * first byte is lsb of 2bit color in column
 * second byte is msb of 2bit color in column
 * so one word of data equals 8 pixels i 2bits color depth packed
 * verticaly
 */
lcd_write_data:
    movem.l (4, %sp), %a0	     /* Data pointer */
    move.l  (8, %sp), %d0            /* Length i in words */
    lea.l   LCD_BASE_ADDRESS+2, %a1  /* LCD data port address */

    btst    #0, %d0    /* longwords multiply? */
    jeq    .l_write

.w_write:
    move.w  (%a0)+, %d1  /* load data         3 cycles*/
    move.b  %d1, (%a1)   /* first byte        1 cycle*/
    lsr.l   #8, %d1      /* load second byte  1 cycle*/
    move.b  %d1, (%a1)   /* transfer          1 cycle*/
    subq.l  #1, %d0      /* decrement counter 1 cycle*/
    jeq     .write_end   
    
.l_write:
    move.l  (%a0)+, %d1  /* load data         2 cycles*/
    swap    %d1          /*                   1 cycle */
    move.b  %d1, (%a1)   /* first byte        1 cycle*/
    lsr.l   #8, %d1      /*                   1 cycle */
    move.b  %d1, (%a1)   /* second byte       1 cycle*/
    lsr.l   #8, %d1      /*                   1 cycle */
    move.b  %d1, (%a1)   /* third byte        1 cycle*/
    lsr.l   #8, %d1      /*                   1 cycle */
    move.b  %d1, (%a1)   /* forth byte        1 cycle*/
    subq.l  #2, %d0      /* decrement counter 1 cycle*/
    bne.s   .l_write

.write_end:
    rts
.wd_end:
    .size   lcd_write_data,.wd_end-lcd_write_data

    .global lcd_mono_data
    .type   lcd_mono_data, @function

lcd_mono_data:
    move.l  (4, %sp), %a0       /* p_bytes */
    move.l  (8, %sp), %d0       /* count */
    lea.l   LCD_BASE_ADDRESS+2, %a1  /* LCD data port address */

.md_loop:
    move.b  (%a0)+, %d1
    move.b  %d1, (%a1)    /* byte transfers */
    move.b  %d1, (%a1)
    subq.l  #1, %d0
    bne.s   .md_loop

    rts
.md_end:
    .size   lcd_mono_data,.md_end-lcd_mono_data

    .global     lcd_grey_data
    .type       lcd_grey_data,@function

lcd_grey_data:
    lea.l   (-11*4, %sp), %sp
    movem.l  %d2-%d7/%a2-%a6, (%sp) /* save some registers */
    movem.l (11*4+4, %sp), %a0-%a2  /* values, phases, length */
    add.l   %a2, %a2
    lea.l   (%a1, %a2.l*4), %a2    /* end address */
    lea.l   LCD_BASE_ADDRESS+2, %a3 /* LCD data port address */
    moveq.l #24, %d4            /* shift count */
    move.l  #0x204081, %d5      /* bit shuffle factor */

    moveq.l #8, %d2
    add.l   %a1, %d2
    and.l   #0xfffffff0, %d2    /* first line bound */
    cmp.l   %d2, %a2            /* end address lower than first line bound? */
    bhs.s   1f
    move.l  %a2, %d2            /* -> adjust end address of head loop */
1:
    cmp.l   %a1, %d2
    bls.s   .g_hend

    /* process head pixels */
    movem.l (%a1), %d0-%d1      /* fetch 8 pixel phases */

    move.l  %d0, %d2
    and.l   #0x80808080, %d2    /* %d2 = 0.......1.......2.......3....... */
    eor.l   %d2, %d0
    add.l   (%a0)+, %d0         /* add values to first 4 phases */

    move.l  %d1, %d3
    and.l   #0x80808080, %d3    /* %d3 = 4.......5.......6.......7....... */
    eor.l   %d3, %d1
    add.l   (%a0)+, %d1         /* add values to second 4 phases */

    lsr.l   #4, %d3             /* %d3 = ....4.......5.......6.......7... */
    or.l    %d3, %d2            /* %d2 = 0...4...1...5...2...6...3...7... */
    mulu.l  %d5, %d2            /* %d2 = 01234567123.567.23..67..3...7... */
    not.l   %d2                 /*       negate bits */
    lsr.l   %d4, %d2            /* %d2 = ........................01234567 */

    move.b  %d2, (%a3)          /* transfer first LCD byte */

    movem.l %d0-%d1, (%a1)      /* store 8 new pixel phases */
    addq.l  #8, %a1

    move.b  %d2, (%a3)          /* transfer second LCD byte */

.g_hend:
    cmp.l   %a1, %a2
    bls.w   .g_tend
    subq.l  #8, %a2
    cmp.l   %a1, %a2
    bls.s   .g_lend

.g_line_loop:
    /* loop that utilizes line transfers */
    movem.l (%a1), %d0-%d3      /* fetch 2 * 8 pixels phases */

    move.l  %d0, %d6
    and.l   #0x80808080, %d6    /* %d6 = 0.......1.......2.......3....... */
    eor.l   %d6, %d0

    move.l  %d1, %d7
    and.l   #0x80808080, %d7    /* %d7 = 4.......5.......6.......7....... */
    eor.l   %d7, %d1

    lsr.l   #4, %d7             /* %d7 = ....4.......5.......6.......7... */
    or.l    %d7, %d6            /* %d6 = 0...4...1...5...2...6...3...7... */
    mulu.l  %d5, %d6            /* %d6 = 01234567123.567.23..67..3...7... */
    not.l   %d6                 /*       negate bits */
    lsr.l   %d4, %d6            /* %d6 = ........................01234567 */

    move.b  %d6, (%a3)          /* transfer first LCD byte */

    movem.l (%a0), %d7/%a4-%a6  /* fetch 2 * 8 pixel values */
    lea.l   (16, %a0), %a0

    move.b  %d6, (%a3)          /* transfer second LCD byte */

    add.l   %d7, %d0
    add.l   %a4, %d1

    move.l  %d2, %d6
    and.l   #0x80808080, %d6    /* %d6 = 0.......1.......2.......3....... */
    eor.l   %d6, %d2

    move.l  %d3, %d7
    and.l   #0x80808080, %d7    /* %d7 = 4.......5.......6.......7....... */
    eor.l   %d7, %d3

    lsr.l   #4, %d7             /* %d7 = ....4.......5.......6.......7... */
    or.l    %d7, %d6            /* %d6 = 0...4...1...5...2...6...3...7... */
    mulu.l  %d5, %d6            /* %d6 = 01234567123.567.23..67..3...7... */
    not.l   %d6                 /*       negate bits */
    lsr.l   %d4, %d6            /* %d6 = ........................01234567 */

    add.l   %a5, %d2
    add.l   %a6, %d3

    move.b  %d6, (%a3)          /* transfer first LCD byte */

    movem.l %d0-%d3, (%a1)      /* store 2 * 8 new pixel phases */
    lea.l   (16, %a1), %a1      /* advance pointer */

    move.b  %d6, (%a3)          /* transfer second LCD byte */

    cmp.l   %a2, %a1
    bls.s   .g_line_loop

.g_lend:
    addq.l  #8, %a2
    cmp.l   %a1, %a2
    bls.s   .g_tend

    /* process tail pixels */
    movem.l (%a1), %d0-%d1      /* fetch 8 pixel phases */

    move.l  %d0, %d2
    and.l   #0x80808080, %d2    /* %d2 = 0.......1.......2.......3....... */
    eor.l   %d2, %d0
    add.l   (%a0)+, %d0         /* add values to first 4 phases */

    move.l  %d1, %d3
    and.l   #0x80808080, %d3    /* %d3 = 4.......5.......6.......7....... */
    eor.l   %d3, %d1
    add.l   (%a0)+, %d1         /* add values to second 4 phases */

    lsr.l   #4, %d3             /* %d3 = ....4.......5.......6.......7... */
    or.l    %d3, %d2            /* %d2 = 0...4...1...5...2...6...3...7... */
    mulu.l  %d5, %d2            /* %d2 = 01234567123.567.23..67..3...7... */
    not.l   %d2                 /*       negate bits */
    lsr.l   %d4, %d2            /* %d2 = ........................01234567 */

    move.b  %d2, (%a3)          /* transfer first LCD byte */

    movem.l %d0-%d1, (%a1)      /* store 8 new pixel phases */
    /* addq.l  #8, %a1             not needed anymore */

    move.b  %d2, (%a3)          /* transfer second LCD byte */

.g_tend:
    movem.l (%sp), %d2-%d7/%a2-%a6
    lea.l   (11*4, %sp), %sp
    rts
.grey_end:
    .size    lcd_grey_data,.grey_end-lcd_grey_data
