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
    lea.l   (-10*4, %sp), %sp
    movem.l %d2-%d6/%a2-%a6, (%sp)  /* free some registers */
    movem.l (10*4+4, %sp), %a0-%a2  /* values, phases, length */
    lea.l   (%a1, %a2.l*4), %a2     /* end address */
    lea     0xf0008002, %a3     /* LCD data port */

    moveq.l #15, %d3
    add.l   %a1, %d3
    and.l   #0xfffffff0, %d3    /* first line bound */
    move.l  %a2, %d1
    and.l   #0xfffffff0, %d1    /* last line bound */
    cmp.l   %d3, %d1
    bls.w   .g_tloop            /* no lines to copy - jump to tail loop */
    cmp.l   %a1, %d0
    bls.s   .g_lloop            /* no head blocks - jump to line loop */

.g_hloop:
    move.l  (%a1), %d2          /* fetch 4 pixel phases */

    bclr.l  #31, %d2            /* Z = !(p0 & 0x80); p0 &= ~0x80; */
    seq.b   %d0                 /* %d0 = ........................00000000 */
    lsl.l   #2, %d0             /* %d0 = ......................00000000.. */
    bclr.l  #23, %d2            /* Z = !(p1 & 0x80); p1 &= ~0x80; */
    seq.b   %d0                 /* %d0 = ......................0011111111 */
    lsl.l   #2, %d0             /* %d0 = ....................0011111111.. */
    bclr.l  #15, %d2            /* Z = !(p2 & 0x80); p2 &= ~0x80; */
    seq.b   %d0                 /* %d0 = ....................001122222222 */
    lsl.l   #2, %d0             /* %d0 = ..................001122222222.. */
    bclr.l  #7, %d2             /* Z = !(p3 & 0x80); p3 &= ~0x80; */
    seq.b   %d0                 /* %d0 = ..................00112233333333 */
    lsr.l   #6, %d0             /* %d0 = ........................00112233 */
    move.w  %d0, (%a3)          /* write pixel block */
    
    add.l   (%a0)+, %d2         /* add 4 pixel values to the phases */
    move.l  %d2, (%a1)+         /* store new phases, advance pointer */

    cmp.l   %a1, %d3            /* go up to first line bound */
    bhi.s   .g_hloop
    
.g_lloop:
    movem.l (%a1), %d2-%d5      /* fetch 4 blocks of 4 pixel phases each */
    
    bclr.l  #31, %d2            /* calculate first pixel block */
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #23, %d2
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #15, %d2
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #7, %d2
    seq.b   %d0
    lsr.l   #6, %d0

    move.w  %d0, (%a3)          /* write first block to LCD */

    bclr.l  #31, %d3            /* calculate second pixel block */
    seq.b   %d6
    lsl.l   #2, %d6
    bclr.l  #23, %d3
    seq.b   %d6
    lsl.l   #2, %d6
    bclr.l  #15, %d3
    seq.b   %d6
    lsl.l   #2, %d6
    bclr.l  #7, %d3
    seq.b   %d6
    lsr.l   #6, %d6

    bclr.l  #31, %d4            /* calculate third pixel block */
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #23, %d4
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #15, %d4
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #7, %d4
    seq.b   %d0
    lsr.l   #6, %d0

    move.w  %d6, (%a3)          /* write second block to LCD */

    movem.l (%a0), %d6/%a4-%a6  /* fetch 4 blocks of 4 pixel values each */
    lea.l   (16, %a0), %a0

    move.w  %d0, (%a3)          /* write third block to LCD */

    bclr.l  #31, %d5            /* calculate fourth pixel block */
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #23, %d5
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #15, %d5
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #7, %d5
    seq.b   %d0
    lsr.l   #6, %d0

    add.l   %d6, %d2            /* calculate 4*4 new pixel phases */
    add.l   %a4, %d3            /* (packed addition) */
    add.l   %a5, %d4
    add.l   %a6, %d5

    movem.l %d2-%d5, (%a1)      /* store 4*4 new pixel phases */
    lea.l   (16, %a1), %a1

    move.w  %d0, (%a3)          /* write fourth block to LCD */

    cmp.l   %a1, %d1            /* go up to last line bound */
    bhi.w   .g_lloop

    cmp.l   %a1, %a2
    bls.s   .g_no_tail

.g_tloop:
    move.l  (%a1), %d2

    bclr.l  #31, %d2
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #23, %d2
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #15, %d2
    seq.b   %d0
    lsl.l   #2, %d0
    bclr.l  #7, %d2
    seq.b   %d0
    lsr.l   #6, %d0
    move.w  %d0, (%a3)

    add.l   (%a0)+, %d2         /* go up to end address */
    move.l  %d2, (%a1)+

    cmp.l   %a1, %a2
    bhi.s   .g_tloop

.g_no_tail:
    movem.l (%sp), %d2-%d6/%a2-%a6  /* restore registers */
    lea.l   (10*4, %sp), %sp
    rts

.gd_end:
    .size   lcd_grey_data,.gd_end-lcd_grey_data
