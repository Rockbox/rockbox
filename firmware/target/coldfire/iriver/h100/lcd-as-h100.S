/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
 * Based on the work of Alan Korr and Jörg Hohensohn
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
    move.l  #~8, %d1
    and.l   %d1, (MBAR2+0xb4)
    move.l  (4, %sp), %d0
    move.w  %d0, 0xf0000000
    rts
.wc_end:
    .size   lcd_write_command,.wc_end-lcd_write_command


    .align      2
    .global     lcd_write_command_ex
    .type       lcd_write_command_ex,@function

lcd_write_command_ex:
    lea.l   0xf0000000, %a0
    lea.l   MBAR2+0xb4, %a1

    move.l  #~8, %d1            /* Set A0 = 0 */
    and.l   %d1, (%a1)

    move.l  (4, %sp), %d0       /* Command */
    move.w  %d0, (%a0)          /* Write to LCD */

    not.l   %d1                 /* Set A0 = 1 */
    or.l    %d1, (%a1)

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
    moveq   #8, %d1
    or.l    %d1, (MBAR2+0xb4)
    lea.l   0xf0000000, %a1

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

lcd_grey_data:
    lea.l   (-4*4, %sp), %sp
    movem.l %d2-%d5, (%sp)
    movem.l (4*4+4, %sp), %a0-%a1 /* Data pointer */
    move.l  %a1, %d0            /* Length */
    moveq   #8, %d1
    or.l    %d1, (MBAR2+0xb4)   /* A0 = 1 (data) */
    lea     0xf0000000, %a1     /* LCD data port */
    move.l  #0xff00ff00, %d2    /* mask for splitting value/phase pairs */

.greyloop:
    movem.l (%a0), %d4-%d5      /* fetch 4 pixel phase/value pairs at once */
                                /* %d4 = p0v0p1v1, %d5 = p2v2p3v3 */
    move.l  %d2, %d3            /* copy mask */
    and.l   %d4, %d3            /* %d3 = p0--p1-- */
    eor.l   %d3, %d4            /* %d4 = --v0--v1 */
    lsr.l   #8, %d3             /* %d3 = --p0--p1 */

    bclr.l  #23, %d3            /* Z = !(p0 & 0x80); p0 &= ~0x80; */
    seq.b   %d1                 /* %d1 = ........................00000000 */
    lsl.l   #2, %d1             /* %d1 = ......................00000000.. */
    bclr.l  #7, %d3             /* Z = !(p1 & 0x80); p1 &= ~0x80; */
    seq.b   %d1                 /* %d1 = ......................0011111111 */
    lsl.l   #2, %d1             /* %d1 = ....................0011111111.. */
    
    add.l   %d4, %d3            /* p0 += v0; p1 += v1; */
    move.b  %d3, (2, %a0)       /* store p1 */
    swap    %d3
    move.b  %d3, (%a0)          /* store p0 */

    move.l  %d2, %d3            /* copy mask */
    and.l   %d5, %d3            /* %d3 = p2--p3-- */
    eor.l   %d3, %d5            /* %d5 = --v2--v3 */
    lsr.l   #8, %d3             /* %d3 = --p2--p3 */

    bclr.l  #23, %d3            /* Z = !(p2 & 0x80); p2 &= ~0x80; */
    seq.b   %d1                 /* %d1 = ....................001122222222 */
    lsl.l   #2, %d1             /* %d1 = ..................001122222222.. */
    bclr.l  #7, %d3             /* Z = !(p3 & 0x80); p3 &= ~0x80; */
    seq.b   %d1                 /* %d1 = ..................00112233333333 */
    lsr.l   #6, %d1             /* %d1 = ........................00112233 */

    add.l   %d5, %d3            /* p2 += v2; p3 += v3; */
    move.b  %d3, (6, %a0)       /* store p3 */
    swap    %d3
    move.b  %d3, (4, %a0)       /* store p2 */

    move.w  %d1, (%a1)          /* write pixel block */
    addq.l  #8, %a0             /* advance address pointer */
    subq.l  #1, %d0             /* any blocks left? */
    bne.b   .greyloop

    movem.l (%sp), %d2-%d5
    lea.l   (4*4, %sp), %sp
    rts
.gd_end:
    .size   lcd_grey_data,.gd_end-lcd_grey_data
