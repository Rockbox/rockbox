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

    move.l  (4, %sp), %d0   /* Command */
    move.w  %d0, (%a0)+     /* Write to LCD, set A0 = 1 */

    move.l  (8, %sp), %d0   /* Data */
    cmp.l   #-1, %d0        /* -1? */
    beq.b   .last
    move.w  %d0, (%a0)      /* Write to LCD */

    move.l  (12, %sp), %d0  /* Data */
    cmp.l   #-1, %d0        /* -1? */
    beq.b   .last
    move.w  %d0, (%a0)      /* Write to LCD */

.last:
    rts
.wcex_end:
    .size   lcd_write_command_ex,.wcex_end-lcd_write_command_ex


    .align      2
    .global     lcd_write_data
    .type       lcd_write_data,@function

lcd_write_data:
    move.l  (4,%sp), %a0    /* Data pointer */
    move.l  (8,%sp), %d0    /* Length */

    lea     0xf0008002, %a1
.loop:
    /* When running in IRAM, this loop takes 10 cycles plus the LCD write.
       The 10 cycles are necessary to follow the LCD timing specs
       at 140MHz */
    nop                     /* 3(0/0) */
    move.b  (%a0)+, %d1     /* 3(1/0) */
    move.w  %d1, (%a1)      /* 1(0/1) */
    subq.l  #1, %d0         /* 1(0/0) */
    bne     .loop           /* 2(0/0) */
    rts
.wd_end:
    .size   lcd_write_data,.wd_end-lcd_write_data
