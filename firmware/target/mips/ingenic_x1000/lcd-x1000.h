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

#include <stdbool.h>

/* Note: this API is subject to change when DMA gets implemented
 *
 * Targets need to define the following functions:
 *
 * - lcd_tgt_power(): turn the panel power on and off
 * - lcd_tgt_ctl_on(): initialize LCD controller or shut it down
 * - lcd_tgt_ctl_sleep(): enter/exit sleep mode (power saving state)
 * - lcd_tgt_set_fb_addr(): set framebuffer address in register mode
 *
 * When powering on the LCD in lcd_tgt_power(), you also need to call
 * lcd_set_clock() to set the pixel clock rate appropriately.
 *
 * In all functions except lcd_tgt_power(), use lcd_exec_commands() to
 * issue commands to the LCD controller. You can use a mix of commands,
 * data, and delays in your instruction sequence.
 *
 * Targets must also provide an lcd-target.h header which defines:
 *
 * - LCD_TGT_BUS_WIDTH: width of data bus in bits
 * - LCD_TGT_CMD_WIDTH: width of commands in bits
 * - LCD_TGT_6800_MODE: 1 for 6800 mode, 0 for 8080 mode
 * - LCD_TGT_SERIAL: 1 for serial interfaces, 0 for parallel interfaces
 * - LCD_TGT_CLK_POLARITY: 0 -> active edge falling, 1 -> active edge rising
 * - LCD_TGT_DC_POLARITY: 0 -> DC is 0 for commands, 1 -> DC is 1 for commands
 * - LCD_TGT_WR_POLARITY: 0 -> keep WR high on idle, 1 -> keep WR low on idle
 */

#define LCD_STATE_OFF   0
#define LCD_STATE_RUN   1
#define LCD_STATE_IDLE  2
#define LCD_STATE_SLEEP 3

#define LCD_INSTR_CMD       0
#define LCD_INSTR_DAT       1
#define LCD_INSTR_UDELAY    2
#define LCD_INSTR_END       3

extern int lcd_get_state(void);
extern void lcd_set_clock(unsigned freq);
extern void lcd_exec_commands(const unsigned* cmdseq);

extern void lcd_tgt_power(bool on);
extern void lcd_tgt_ctl_on(bool on);
extern void lcd_tgt_ctl_sleep(bool sleep);
extern void lcd_tgt_set_fb_addr(int x1, int y1, int x2, int y2);

#endif /* __LCD_X1000_H__ */
