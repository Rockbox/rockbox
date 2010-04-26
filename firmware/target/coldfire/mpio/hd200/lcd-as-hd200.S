/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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
    move.w  %d0, LCD_BASE_ADDRESS	/* data is 1byte but CF uses word
                                         * transfers only */
    rts
.wc_end:
    .size   lcd_write_command,.wc_end-lcd_write_command


    .align      2
    .global     lcd_write_command_e
    .type       lcd_write_command_e,@function

lcd_write_command_e:
    lea.l   LCD_BASE_ADDRESS, %a0

    move.l  (4, %sp), %d0       /* Command */
    move.w  %d0, (%a0)         
    move.l  (8, %sp), %d0       /* Data */
    move.w  %d0, (%a0)          /* Write to LCD */

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
    lea     LCD_BASE_ADDRESS+2, %a1  /* LCD data port address */

    btst    #0, %d0    /* longwords multiply? */
    beq    .l_write

.w_write:
    move.w  (%a0)+, %d1  /* load data         3 cycles*/
    move.w  %d1, (%a1)   /* first byte        1 cycle*/
    lsr.l   #8, %d1      /* load second byte  1 cycle*/
    move.w  %d1, (%a1)   /* transfer          1 cycle*/
    subq.l  #1, %d0      /* decrement counter 1 cycle*/
    beq     .write_end   
    
.l_write:
    move.l  (%a0)+, %d1  /* load data         2 cycles*/
    swap    %d1          /*                   1 cycle */
    move.w  %d1, (%a1)   /* first byte        1 cycle*/
    lsr.l   #8, %d1      /*                   1 cycle */
    move.w  %d1, (%a1)   /* second byte       1 cycle*/
    lsr.l   #8, %d1      /*                   1 cycle */
    move.w  %d1, (%a1)   /* third byte        1 cycle*/
    lsr.l   #8, %d1      /*                   1 cycle */
    move.w  %d1, (%a1)   /* forth byte        1 cycle*/
    subq.l  #2, %d0      /* decrement counter 1 cycle*/
    bne     .l_write

.write_end:
    rts
    .size   lcd_write_data,.wd_end-lcd_write_data

