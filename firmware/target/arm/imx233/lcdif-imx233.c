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
#include "lcdif-imx233.h"

static unsigned lcdif_word_length = 0;
static unsigned lcdif_byte_packing = 0;

void imx233_lcdif_enable_bus_master(bool enable)
{
    if(enable)
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__LCDIF_MASTER;
    else
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__LCDIF_MASTER;
}

void imx233_lcdif_enable(bool enable)
{
    if(enable)
        __REG_CLR(HW_LCDIF_CTRL) = __BLOCK_CLKGATE;
    else
        __REG_SET(HW_LCDIF_CTRL) = __BLOCK_CLKGATE;
}

void imx233_lcdif_reset(void)
{
    //imx233_reset_block(&HW_LCDIF_CTRL);// doesn't work
    while(HW_LCDIF_CTRL & __BLOCK_CLKGATE)
        HW_LCDIF_CTRL &= ~__BLOCK_CLKGATE;
    while(!(HW_LCDIF_CTRL & __BLOCK_SFTRST))
        HW_LCDIF_CTRL |= __BLOCK_SFTRST;
    while(HW_LCDIF_CTRL & __BLOCK_CLKGATE)
        HW_LCDIF_CTRL &= ~__BLOCK_CLKGATE;
    while(HW_LCDIF_CTRL & __BLOCK_SFTRST)
        HW_LCDIF_CTRL &= ~__BLOCK_SFTRST;
    while(HW_LCDIF_CTRL & __BLOCK_CLKGATE)
        HW_LCDIF_CTRL &= ~__BLOCK_CLKGATE;
    __REG_SET(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__RESET;
}

void imx233_lcdif_set_timings(unsigned data_setup, unsigned data_hold,
    unsigned cmd_setup, unsigned cmd_hold)
{
    HW_LCDIF_TIMING = (data_setup << HW_LCDIF_TIMING__DATA_SETUP_BP) |
        (data_hold << HW_LCDIF_TIMING__DATA_HOLD_BP) |
        (cmd_setup << HW_LCDIF_TIMING__CMD_SETUP_BP) |
        (cmd_hold << HW_LCDIF_TIMING__CMD_HOLD_BP);
}

void imx233_lcdif_set_lcd_databus_width(unsigned width)
{
    __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__LCD_DATABUS_WIDTH_BM;
    __REG_SET(HW_LCDIF_CTRL) = width;
}

void imx233_lcdif_set_word_length(unsigned word_length)
{
    __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__WORD_LENGTH_BM;
    __REG_SET(HW_LCDIF_CTRL) = word_length;
    lcdif_word_length = word_length;
}

unsigned imx233_lcdif_enable_irqs(unsigned irq_bm)
{
    unsigned old_msk = (HW_LCDIF_CTRL1 & HW_LCDIF_CTRL1__IRQ_EN_BM) >>HW_LCDIF_CTRL1__IRQ_EN_BP ;
    /* clear irq status */
    __REG_CLR(HW_LCDIF_CTRL1) = irq_bm << HW_LCDIF_CTRL1__IRQ_BP;
    /* disable irqs */
    __REG_CLR(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__IRQ_EN_BM;
    /* enable irqs */
    __REG_SET(HW_LCDIF_CTRL1) = irq_bm << HW_LCDIF_CTRL1__IRQ_EN_BP;

    return old_msk;
}

void imx233_lcdif_set_byte_packing_format(unsigned byte_packing)
{
    __REG_CLR(HW_LCDIF_CTRL1) = HW_LCDIF_CTRL1__BYTE_PACKING_FORMAT_BM;
    __REG_SET(HW_LCDIF_CTRL1) = byte_packing << HW_LCDIF_CTRL1__BYTE_PACKING_FORMAT_BP;
    lcdif_byte_packing = byte_packing;
}

void imx233_lcdif_set_data_format(bool data_fmt_16, bool data_fmt_18, bool data_fmt_24)
{
    if(data_fmt_16)
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_FORMAT_16_BIT;
    else
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_FORMAT_16_BIT;
    if(data_fmt_18)
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_FORMAT_18_BIT;
    else
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_FORMAT_18_BIT;
    if(data_fmt_24)
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_FORMAT_24_BIT;
    else
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_FORMAT_24_BIT;
}

void imx233_lcdif_wait_ready(void)
{
    while(HW_LCDIF_CTRL & HW_LCDIF_CTRL__RUN);
}

void imx233_lcdif_pio_send(bool data_mode, unsigned len, uint32_t *buf)
{
    unsigned max_xfer_size = 0xffff;
    if(len == 0)
        return;
    if(lcdif_word_length == HW_LCDIF_CTRL__WORD_LENGTH_16_BIT)
        max_xfer_size = 0x1fffe;
    imx233_lcdif_wait_ready();
    unsigned msk = imx233_lcdif_enable_irqs(0);
    imx233_lcdif_enable_bus_master(false);

    do
    {
        unsigned burst = MIN(len, max_xfer_size);
        len -= burst;
        unsigned count = burst;
        if(lcdif_word_length != HW_LCDIF_CTRL__WORD_LENGTH_8_BIT)
        {
            if(burst & 1)
                burst++;
            count = burst / 2;
        }
        else
            count = burst;
        HW_LCDIF_TRANSFER_COUNT = 0;
        HW_LCDIF_TRANSFER_COUNT = 0x10000 | count;
        __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_SELECT | HW_LCDIF_CTRL__RUN;
        if(data_mode)
            __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_SELECT;
        __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__RUN;
        burst = (burst + 3) / 4;
        while(burst-- > 0)
        {
            while(HW_LCDIF_STAT & HW_LCDIF_STAT__LFIFO_FULL);
            HW_LCDIF_DATA = *buf++;
        }
        while(HW_LCDIF_CTRL & HW_LCDIF_CTRL__RUN);
    }while(len > 0);
    imx233_lcdif_enable_bus_master(true);
    imx233_lcdif_enable_irqs(msk);
}

void imx233_lcdif_dma_send(void *buf, unsigned width, unsigned height)
{
    HW_LCDIF_CUR_BUF = (uint32_t)buf;
    HW_LCDIF_TRANSFER_COUNT = 0;
    HW_LCDIF_TRANSFER_COUNT = (height << 16) | width;
    __REG_CLR(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__RUN;
    __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__DATA_SELECT;
    __REG_SET(HW_LCDIF_CTRL) = HW_LCDIF_CTRL__RUN;
}
