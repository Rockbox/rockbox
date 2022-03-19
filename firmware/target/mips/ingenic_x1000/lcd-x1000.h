/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __LCD_X1000_H__
#define __LCD_X1000_H__

/* NOTICE: if adding LCD support for a new X1000 target, please take a look
 * at the implementation in case there's any difficulties; there may be some
 * parts that need adjusting. The X1000 LCD interface is poorly documented
 * and it might be necessary to change some settings by trial and error to
 * match the panel. */

#include "clk-x1000.h"
#include <stdbool.h>

#define LCD_INSTR_CMD       0
#define LCD_INSTR_DAT       1
#define LCD_INSTR_UDELAY    2
#define LCD_INSTR_END       3

struct lcd_tgt_config {
    /* Data bus width, in bits */
    unsigned bus_width: 8;

    /* Command bus width, in bits */
    unsigned cmd_width: 8;

    /* 1 = use 6800 timings, 0 = use 8080 timings */
    unsigned use_6800_mode: 1;

    /* 1 = serial interface, 0 = parallel interface */
    unsigned use_serial: 1;

    /* Clock active edge: 0 = falling edge, 1 = rising edge */
    unsigned clk_polarity: 1;

    /* DC pin levels: 1 = data high, command low; 0 = data low, command high */
    unsigned dc_polarity: 1;

    /* WR pin level during idle: 1 = keep high; 0 = keep low */
    unsigned wr_polarity: 1;

    /* 1 to enable vsync, so DMA transfer is synchronized with TE signal */
    unsigned te_enable: 1;

    /* Active level of TE signal: 1 = high, 0 = low */
    unsigned te_polarity: 1;

    /* 1 = support narrow TE signal (<=3 pixel clocks); 0 = don't support */
    unsigned te_narrow: 1;

    /* 1 = big endian mode, 0 = little endian mode */
    unsigned big_endian: 1;

    /* Commands used to initiate a framebuffer write. Buffer must be
     * aligned to 64-byte boundary and size must be a multiple of 4,
     * regardless of the command bus width. */
    const void* dma_wr_cmd_buf;
    size_t dma_wr_cmd_size;
};

/* Static configuration for the target's LCD, must be defined by target. */
extern const struct lcd_tgt_config lcd_tgt_config;

/* Set the pixel clock. Valid clock sources are SCLK_A and MPLL. */
extern void lcd_set_clock(x1000_clk_t clksrc, uint32_t freq);

/* Execute a sequence of LCD commands. Should only be called from
 * lcd_tgt_ctl_enable() and lcd_tgt_ctl_sleep().
 *
 * The array should be a list of pairs (instr, arg), with LCD_INSTR_END
 * as the last entry.
 *
 * - LCD_INSTR_CMD, cmd: issue command write of 'cmd'
 * - LCD_INSTR_DAT, dat: issue data write of 'dat'
 * - LCD_INSTR_UDELAY, us: call udelay(us)
 */
extern void lcd_exec_commands(const uint32_t* cmdseq);

/* Enable/disable the LCD controller.
 *
 * - On enabling: power on the LCD, set the pixel clock with lcd_set_clock(),
 *   and use lcd_exec_commands() to send any needed initialization commands.
 *
 * - On disabling: use lcd_exec_commands() to send shutdown commands to the
 *   controller and disable the LCD power supply.
 */
extern void lcd_tgt_enable(bool on);

/* Enable/disable the LCD controller, but intended for booting the OF.
 *
 * This is only used for the Eros Q Native port, as the OF seems to be
 * unable to initialize the LCD in the kernel boot rather than having
 * the bootloader do it.
 */
extern void lcd_tgt_enable_of(bool on);

/* Enter or exit sleep mode to save power, normally by sending the necessary
 * commands with lcd_exec_commands().
 */
extern void lcd_tgt_sleep(bool sleep);

#endif /* __LCD_X1000_H__ */
