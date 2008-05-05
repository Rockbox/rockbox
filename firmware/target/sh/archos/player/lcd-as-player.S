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

#define LCD_DS  1 /* PB0 = 1 --- 0001 ---  LCD-DS */
#define LCD_CS  2 /* PB1 = 1 --- 0010 --- /LCD-CS */
#define LCD_SD  4 /* PB2 = 1 --- 0100 ---  LCD-SD */
#define LCD_SC  8 /* PB3 = 1 --- 1000 ---  LCD-SC */

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
 *   r4 - command byte (int)
 *
 * Register usage:
 *   r0 - scratch
 *   r1 - command byte (copied)
 *   r2 - precalculated port value (CS, DS and SC low, SD high)
 *   r3 - lcd port address
 *   r5 - 1 (byte count for reuse of the loop in _lcd_write_data)
 */

_lcd_write_command:
    mov.l   .lcdr, r3       /* put lcd data port address in r3 */
    mov     r4, r1          /* copy data byte to r1 */
    mov     #0, r5          /* fake end address - stop after first iteration */

    /* This code will fail if an interrupt changes the contents of PBDRL.
     * If so, we must disable the interrupt here. */

    mov.b   @r3, r0         /* r0 = PBDRL */
    or      #(LCD_SD), r0   /* r0 |= LCD_SD */
    and     #(~(LCD_CS|LCD_DS|LCD_SC)),r0  /* r0 &= ~(LCD_CS|LCD_DS|LCD_SC) */

    bra     .single_transfer /* jump into the transfer loop */
    mov     r0, r2


    .align      2
    .global     _lcd_write_command_e
    .type       _lcd_write_command_e,@function

/* Write a command byte and a data byte to the lcd controller
 *
 * Arguments:
 *   r4 - command byte
 *   r5 - data byte
 *
 * Register usage:
 *   r0 - scratch
 *   r1 - command/data byte (copied)
 *   r2 - precalculated port value (CS, DS and SC low, SD high)
 *   r3 - lcd port address
 *   r5 - fake end address
 *   r6 - data byte (saved)
 *   r7 - saved pr
 */

_lcd_write_command_e:
    mov.l   .lcdr, r3       /* put lcd data port address in r3 */
    mov     r4, r1          /* copy data byte to r1 */
    mov     r5, r6
    mov     #0, r5          /* fake end address - stop after first iteration */

    /* This code will fail if an interrupt changes the contents of PBDRL.
     * If so, we must disable the interrupt here. */

    mov.b   @r3, r0         /* r0 = PBDRL */
    or      #(LCD_SD), r0   /* r0 |= LCD_SD */
    and     #(~(LCD_CS|LCD_DS|LCD_SC)),r0  /* r0 &= ~(LCD_CS|LCD_DS|LCD_SC) */

    sts     pr, r7
    bsr     .single_transfer /* jump into the transfer loop */
    mov     r0, r2

    lds     r7, pr
    mov     r6, r1
    or      #(LCD_DS|LCD_SD), r0    /* r0 |= LCD_DS|LCD_SD */
    and     #(~(LCD_CS|LCD_SC)), r0 /* r0 &= ~(LCD_CS|LCD_SC) */
    bra     .single_transfer /* jump into the transfer loop */
    mov     r0, r2


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
 *   r2 - precalculated port value (CS and SC low, DS and SD high)
 *   r3 - lcd port address
 *   r4 - current address
 *   r5 - end address
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
    mov     r0, r2

    .align  2
.multi_transfer:
    mov.b   @r4+, r1        /* load data byte from memory */

.single_transfer:
    shll16  r1              /* shift data to most significant byte */
    shll8   r1

    shll    r1              /* shift the msb into carry */
    mov     r2, r0          /* copy precalculated port value */
    bt      1f              /* data bit = 1? */
    and     #(~LCD_SD), r0  /* no: r0 &= ~LCD_SD */
1:
    shll    r1              /* next shift here for alignment */
    mov.b   r0, @r3         /* set data to port */
    or      #(LCD_SC), r0   /* rise SC (independent of SD level) */
    mov.b   r0, @r3         /* set to port */

    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    shll    r1
    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    shll    r1
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    shll    r1
    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    shll    r1
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    shll    r1
    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    shll    r1
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    mov     r2, r0
    bt      1f
    and     #(~LCD_SD), r0
1:
    mov.b   r0, @r3
    or      #(LCD_SC), r0
    mov.b   r0, @r3

    cmp/hi  r4, r5          /* some blocks left? */
    bt      .multi_transfer

    or      #(LCD_CS|LCD_DS|LCD_SD|LCD_SC),r0  /* restore port */
    rts
    mov.b   r0, @r3

    /* This is the place to reenable the interrupts, if we have disabled
     * them. See above. */

    .align  2
.lcdr:
    .long   LCDR
