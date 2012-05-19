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

#define HW_LCDIF_BASE           0x80030000

#define HW_LCDIF_CTRL           (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x0))
#define HW_LCDIF_CTRL__WORD_LENGTH_16_BIT   (0 << 8)
#define HW_LCDIF_CTRL__WORD_LENGTH_8_BIT    (1 << 8)
#define HW_LCDIF_CTRL__WORD_LENGTH_18_BIT   (2 << 8)
#define HW_LCDIF_CTRL__WORD_LENGTH_24_BIT   (3 << 8)
#define HW_LCDIF_CTRL__WORD_LENGTH_BM       (3 << 8)
#define HW_LCDIF_CTRL__LCD_DATABUS_WIDTH_16_BIT (0 << 10)
#define HW_LCDIF_CTRL__LCD_DATABUS_WIDTH_18_BIT (2 << 10)
#define HW_LCDIF_CTRL__LCD_DATABUS_WIDTH_BM (3 << 10)
#define HW_LCDIF_CTRL__LCDIF_MASTER         (1 << 5)
#define HW_LCDIF_CTRL__DATA_FORMAT_16_BIT   (1 << 3)
#define HW_LCDIF_CTRL__DATA_FORMAT_18_BIT   (1 << 2)
#define HW_LCDIF_CTRL__DATA_FORMAT_24_BIT   (1 << 1)
#define HW_LCDIF_CTRL__RUN                  0x1
#define HW_LCDIF_CTRL__DATA_SELECT          (1 << 16)

#define HW_LCDIF_CTRL1          (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x10))
#define HW_LCDIF_CTRL1__RESET           1
#define HW_LCDIF_CTRL1__BUSY_ENABLE     (1 << 2)
#define HW_LCDIF_CTRL1__MODE86          (1 << 1)
#define HW_LCDIF_CTRL1__IRQ_EN_BP       12
#define HW_LCDIF_CTRL1__IRQ_EN_BM       (0xf << 12)
#define HW_LCDIF_CTRL1__IRQ_BP          8
#define HW_LCDIF_CTRL1__BYTE_PACKING_FORMAT_BM  (0xf << 16)
#define HW_LCDIF_CTRL1__BYTE_PACKING_FORMAT_BP  16

#define HW_LCDIF__VSYNC_EDGE_IRQ        1
#define HW_LCDIF__CUR_FRAME_DONE_IRQ    2
#define HW_LCDIF__UNDERFLOW_IRQ         4
#define HW_LCDIF__OVERFLOW_IRQ          8

#define HW_LCDIF_TRANSFER_COUNT (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x20))
#define HW_LCDIF_CUR_BUF        (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x30))
#define HW_LCDIF_NEXT_BUF       (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x40))
#define HW_LCDIF_TIMING         (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x60))
#define HW_LCDIF_TIMING__DATA_SETUP_BP  0
#define HW_LCDIF_TIMING__DATA_HOLD_BP   8
#define HW_LCDIF_TIMING__CMD_SETUP_BP   16
#define HW_LCDIF_TIMING__CMD_HOLD_BP    24

#define HW_LCDIF_CSC_COEFF0     (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x110))
#define HW_LCDIF_CSC_COEFF1     (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x120))
#define HW_LCDIF_CSC_COEFF2     (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x130))
#define HW_LCDIF_CSC_COEFF3     (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x140))
#define HW_LCDIF_CSC_COEFF4     (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x150))
#define HW_LCDIF_CSC_OFFSET     (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x160))
#define HW_LCDIF_CSC_LIMIT      (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x170))
#define HW_LCDIF_DATA           (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x1b0))

#define HW_LCDIF_STAT           (*(volatile uint32_t *)(HW_LCDIF_BASE + 0x1d0))
#define HW_LCDIF_STAT__LFIFO_FULL   (1 << 29)
#define HW_LCDIF_STAT__LFIFO_EMPTY  (1 << 28)
#define HW_LCDIF_STAT__TXFIFO_FULL  (1 << 27)
#define HW_LCDIF_STAT__TXFIFO_EMPTY (1 << 26)
#define HW_LCDIF_STAT__BUSY         (1 << 25)

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
