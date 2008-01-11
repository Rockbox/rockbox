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

#define LCDR (PBDR_ADDR+1)

#define LCD_SD  1 /* PB0 = 1 --- 0001 */
#define LCD_SC  2 /* PB1 = 1 --- 0010 */
#define LCD_DS  4 /* PB2 = 1 --- 0100 */
#define LCD_CS  8 /* PB3 = 1 --- 1000 */

/*
 * About /CS,DS,SC,SD
 * ------------------
 *
 * LCD on JBP and JBR uses a SPI protocol to receive orders (SDA and SCK lines)
 *
 * - /CS -> Chip Selection line :
 *            0 : LCD chipset is activated.
 * -  DS -> Data Selection line, latched at the rising edge
 *          of the 8th serial clock (*) :
 *            0 : instruction register,
 *            1 : data register;
 * -  SC -> Serial Clock line (SDA).
 * -  SD -> Serial Data line (SCK), latched at the rising edge
 *          of each serial clock (*).
 *
 *    _                                                          _
 * /CS \                                                        /
 *      \______________________________________________________/
 *    _____  ____  ____  ____  ____  ____  ____  ____  ____  _____
 *  SD     \/ D7 \/ D6 \/ D5 \/ D4 \/ D3 \/ D2 \/ D1 \/ D0 \/
 *    _____/\____/\____/\____/\____/\____/\____/\____/\____/\_____
 *
 *    _____     _     _     _     _     _     _     _     ________
 *  SC     \   * \   * \   * \   * \   * \   * \   * \   *
 *          \_/   \_/   \_/   \_/   \_/   \_/   \_/   \_/
 *    _  _________________________________________________________
 *  DS \/
 *    _/\_________________________________________________________
 *
 */

    .section    .icode,"ax",@progbits

    .align      2
    .global     _lcd_write_command
    .type       _lcd_write_command,@function

/* Write a command byte to the lcd controller
 *
 * Arguments:
 *   r4 - data byte (int)
 *
 * Register usage:
 *   r0 - scratch
 *   r1 - data byte (copied)
 *   r2 - precalculated port value (CS, DS and SC low, SD high),
 *        negated (neg)!
 *   r3 - lcd port address
 *   r5 - 1 (byte count for reuse of the loop in _lcd_write_data)
 */

_lcd_write_command:
    mov.l   .lcdr, r3       /* put lcd data port address in r3 */
    mov     r4, r1          /* copy data byte to r1 */

    /* This code will fail if an interrupt changes the contents of PBDRL.
     * If so, we must disable the interrupt here. */

    mov.b   @r3, r0         /* r0 = PBDRL */
    mov     r4, r5          /* (fake) end address = current address */
    or      #(LCD_SD), r0   /* r0 |= LCD_SD */
    and     #(~(LCD_CS|LCD_DS|LCD_SC)), r0  /* r0 &= ~(LCD_CS|LCD_DS|LCD_SC) */

    bra     .single_transfer /* jump into the transfer loop */
    neg     r0, r2          /* r2 = 0 - r0 */


    .align      2
    .global     _lcd_write_data
    .type       _lcd_write_data,@function

/* A high performance function to write data to the display,
 * one or multiple bytes.
 *
 * Arguments:
 *   r4 - data address
 *   r5 - byte count
 *
 * Register usage:
 *   r0 - scratch
 *   r1 - current data byte
 *   r2 - precalculated port value (CS and SC low, DS and SD high),
 *        negated (neg)!
 *   r3 - lcd port address
 */

_lcd_write_data:
    mov.l   .lcdr, r3       /* put lcd data port address in r3 */
    add     r4, r5          /* end address */

    /* This code will fail if an interrupt changes the contents of PBDRL.
     * If so, we must disable the interrupt here. If disabling interrupts
     * for a long time (~9200 clks = ~830 µs for transferring 112 bytes on
     * recorders)is undesirable, the loop has to be rewritten to
     * disable/precalculate/transfer/enable for each iteration. However,
     * this would significantly decrease performance. */

    mov.b   @r3, r0         /* r0 = PBDRL */
    or      #(LCD_DS|LCD_SD), r0    /* r0 |= LCD_DS|LCD_SD */
    and     #(~(LCD_CS|LCD_SC)), r0 /* r0 &= ~(LCD_CS|LCD_SC) */
    neg     r0, r2          /* r2 = 0 - r0 */

    /* loop exploits that SD is on bit 0 for recorders and Ondios */

    .align  2
.multi_transfer:
    mov.b   @r4+, r1        /* load data byte from memory */
    nop

.single_transfer:
    shll16  r1              /* shift data to most significant byte */
    shll8   r1
    not     r1, r1          /* and invert for use with negc */

    shll    r1              /* shift the MSB into carry */
    negc    r2, r0          /* carry to SD, SC low */
    shll    r1              /* next shift here for alignment */
    mov.b   r0, @r3         /* set data to port */
    or      #(LCD_SC), r0   /* rise SC (independent of SD level) */
    mov.b   r0, @r3         /* set to port */

    negc    r2, r0
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    shll    r1
    negc    r2, r0
    shll    r1
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    negc    r2, r0
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    shll    r1
    negc    r2, r0
    shll    r1
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    negc    r2, r0
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    shll    r1
    negc    r2, r0
    shll    r1
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    negc    r2, r0
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    cmp/hi  r4, r5          /* some blocks left? */
    bt      .multi_transfer

    or      #(LCD_CS|LCD_DS|LCD_SD|LCD_SC), r0 /* restore port */
    rts
    mov.b   r0, @r3

    /* This is the place to reenable the interrupts, if we have disabled
     * them. See above. */


    .align      2
    .global     _lcd_grey_data
    .type       _lcd_grey_data,@function

/* A high performance function to write grey phase data to the display,
 * one or multiple pixels.
 *
 * Arguments:
 *   r4 - pixel value data address
 *   r5 - pixel phase data address
 *   r6 - pixel block count
 *
 * Register usage:
 *   r0 - scratch / phase signs mask
 *   r1 - scratch
 *   r2 - precalculated port value (CS and SC low, DS and SD high),
 *        negated (neg)!
 *   r3 - lcd port address
 *   r4 - current value address
 *   r5 - current phase address
 *   r6 - end address
 *   r7/r8  - current/next block of phases (alternating)
 *   r9/r10 - current blocks of values
 *   r11 - 0x00000080 \
 *   r12 - 0x00008000  > for phase sign check
 *   r13 - 0x00800000 /
 */

_lcd_grey_data:
    mov.l   r8, @-r15       /* save r8 */
    mov.l   r9, @-r15       /* save r9 */
    mov.l   r10, @-r15      /* save r10 */
    shll2   r6              /*    v */
    mov.l   r11, @-r15      /* save r11 */
    shll    r6              /* r6 *= 8; (8 pixels per block) */
    mov.l   .lcdr, r3       /* put lcd data port address in r3 */
    add     r4, r6          /* end address */

    /* This code will fail if an interrupt changes the contents of PBDRL.
     * If so, we must disable the interrupt here. If disabling interrupts
     * for a long time is undesirable, the loop has to be rewritten to
     * disable/precalculate/transfer/enable for each iteration. However,
     * this would significantly decrease performance. */

    mov.b   @r3, r0         /* r0 = PBDRL */
    or      #(LCD_DS|LCD_SD), r0    /* r0 |= LCD_DS|LCD_SD */
    mov.l   r12, @-r15      /* save r12 */
    and     #(~(LCD_CS|LCD_SC)), r0 /* r0 &= ~(LCD_CS|LCD_SC) */
    mov.l   r13, @-r15      /* save r13 */
    neg     r0, r2          /* r2 = 0 - r0 */

    /* loop exploits that SD is on bit 0 for recorders and Ondios */

    mov.w   .ptest, r11
    swap.b  r11, r12
    mov.l   @r5, r7
    swap.w  r11, r13
    mov.l   .pmask, r0

.greyloop:
    cmp/pz  r7
    mov.l   @r4+, r9
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    tst     r13, r7
    mov.l   @r4+, r10
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    tst     r12, r7
    mov.l   @(4,r5), r8
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    tst     r11, r7
    or      r0, r7
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    cmp/pz  r8
    sub     r9, r7
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    tst     r13, r8
    mov.l   r7, @r5
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    tst     r12, r8
    mov.l   @(8,r5), r7
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    tst     r11, r8
    or      r0, r8
    negc    r2, r1
    mov.b   r1, @r3
    add     #(LCD_SC), r1
    mov.b   r1, @r3

    sub     r10, r8
    mov.l   r8, @(4,r5)
    
    add     #8, r5
    cmp/hi  r4, r6
    bt      .greyloop

    mov.l   @r15+, r13      /* restore r13 */
    mov     #(LCD_CS|LCD_DS|LCD_SD|LCD_SC), r0
    mov.l   @r15+, r12      /* restore r12 */
    or      r0, r1          /* restore port */
    mov.l   @r15+, r11      /* restore r11 */
    mov.l   @r15+, r10      /* restore r10 */
    mov.l   @r15+, r9       /* restore r9 */
    mov.l   @r15+, r8       /* restore r8 */
    rts
    mov.b   r1, @r3

    /* This is the place to reenable the interrupts, if we have disabled
     * them. See above. */

.ptest:
    .short  0x0080

    .align  2
.lcdr:
    .long   LCDR
.pmask:
    .long   0x80808080
