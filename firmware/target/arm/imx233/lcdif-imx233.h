/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2011 by Amaury Pouly
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
#ifndef __LCDIF_IMX233_H__
#define __LCDIF_IMX233_H__

#include <string.h>
#include "cpu.h"
#include "system.h"
#include "system-target.h"

typedef void (*lcdif_irq_cb_t)(void);

void imx233_lcdif_enable(bool enable);
/* WARNING: pixclk must be running before calling lcdif_init */
void imx233_lcdif_init(void);// reset lcdif block
void imx233_lcdif_reset_lcd(bool enable);// set/clr reset line
void imx233_lcdif_set_timings(unsigned data_setup, unsigned data_hold,
    unsigned cmd_setup, unsigned cmd_hold);
void imx233_lcdif_set_word_length(unsigned word_length);
void imx233_lcdif_wait_ready(void);
void imx233_lcdif_set_data_swizzle(unsigned swizzle);
/* This function assumes the data is packed in 8/16-bit mode and unpacked in
 * 18-bit mode.
 * WARNING: doesn't support 24-bit mode
 * WARNING: count must be lower than or equal to 0xffff
 * Note that this function might affect the byte packing format and bus master.
 * Note that count is number of pixels, NOT the number of bytes ! */
void imx233_lcdif_pio_send(bool data_mode, unsigned count, void *buf);
void imx233_lcdif_dma_send(void *buf, unsigned width, unsigned height);
void imx233_lcdif_setup_system_pins(unsigned bus_width);
void imx233_lcdif_setup_dotclk_pins(unsigned bus_width, bool have_enable);

#if IMX233_SUBTARGET >= 3700
void imx233_lcdif_set_byte_packing_format(unsigned byte_packing);
/* low-level function */
void imx233_lcdif_setup_dotclk(unsigned v_pulse_width, unsigned v_period,
    unsigned v_wait_cnt, unsigned v_active, unsigned h_pulse_width,
    unsigned h_period, unsigned h_wait_cnt, unsigned h_active, bool enable_present);
/* high-level function */
void imx233_lcdif_setup_dotclk_ex(unsigned v_pulse_width, unsigned v_back_porch,
    unsigned v_front_porch, unsigned h_pulse_width, unsigned h_back_porch,
    unsigned h_front_porch, unsigned width, unsigned height, unsigned clk_per_pix,
    bool enable_present);
void imx233_lcdif_enable_frame_done_irq(bool en);
void imx233_lcdif_set_frame_done_cb(lcdif_irq_cb_t cb);
void imx233_lcdif_enable_vsync_edge_irq(bool en);
void imx233_lcdif_set_vsync_edge_cb(lcdif_irq_cb_t cb);
void imx233_lcdif_enable_underflow_irq(bool en);
void imx233_lcdif_set_underflow_cb(lcdif_irq_cb_t cb);
void imx233_lcdif_enable_sync_signals(bool en);
#endif

#if IMX233_SUBTARGET >= 3780
void imx233_lcdif_enable_underflow_recover(bool enable);
void imx233_lcdif_enable_bus_master(bool enable);
void imx233_lcdif_set_lcd_databus_width(unsigned width);
#endif

#endif /* __LCDIF_IMX233_H__ */
