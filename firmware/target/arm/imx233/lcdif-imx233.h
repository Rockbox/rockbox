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

#include "regs/regs-lcdif.h"

void imx233_lcdif_enable_underflow_recover(bool enable);
void imx233_lcdif_enable_bus_master(bool enable);
void imx233_lcdif_enable(bool enable);
void imx233_lcdif_reset(void);// reset lcdif block
void imx233_lcdif_set_timings(unsigned data_setup, unsigned data_hold,
    unsigned cmd_setup, unsigned cmd_hold);
void imx233_lcdif_set_lcd_databus_width(unsigned width);
void imx233_lcdif_set_word_length(unsigned word_length);
void imx233_lcdif_set_byte_packing_format(unsigned byte_packing);
void imx233_lcdif_set_data_format(bool data_fmt_16, bool data_fmt_18, bool data_fmt_24);
unsigned imx233_lcdif_enable_irqs(unsigned irq_bm); /* return old mask */
void imx233_lcdif_wait_ready(void);
void imx233_lcdif_pio_send(bool data_mode, unsigned len, uint32_t *buf);
void imx233_lcdif_dma_send(void *buf, unsigned width, unsigned height);

#endif /* __LCDIF_IMX233_H__ */
