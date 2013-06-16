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

void imx233_lcdif_enable_underflow_recover(bool enable)
{
    if(enable)
        BF_SET(LCDIF_CTRL1, RECOVER_ON_UNDERFLOW);
    else
        BF_CLR(LCDIF_CTRL1, RECOVER_ON_UNDERFLOW);
}

void imx233_lcdif_enable_bus_master(bool enable)
{
    if(enable)
        BF_SET(LCDIF_CTRL, LCDIF_MASTER);
    else
        BF_CLR(LCDIF_CTRL, LCDIF_MASTER);
}

void imx233_lcdif_enable(bool enable)
{
    if(enable)
        BF_CLR(LCDIF_CTRL, CLKGATE);
    else
        BF_SET(LCDIF_CTRL, CLKGATE);
}

void imx233_lcdif_reset(void)
{
    imx233_reset_block(&HW_LCDIF_CTRL);
}

void imx233_lcdif_set_timings(unsigned data_setup, unsigned data_hold,
    unsigned cmd_setup, unsigned cmd_hold)
{
    HW_LCDIF_TIMING = BF_OR4(LCDIF_TIMING, DATA_SETUP(data_setup),
        DATA_HOLD(data_hold), CMD_SETUP(cmd_setup), CMD_HOLD(cmd_hold));
}

void imx233_lcdif_set_lcd_databus_width(unsigned width)
{
    BF_WR(LCDIF_CTRL, LCD_DATABUS_WIDTH, width);
}

void imx233_lcdif_set_word_length(unsigned word_length)
{
    BF_WR(LCDIF_CTRL, WORD_LENGTH, word_length);
    lcdif_word_length = word_length;
}

void imx233_lcdif_set_byte_packing_format(unsigned byte_packing)
{
    BF_WR(LCDIF_CTRL1, BYTE_PACKING_FORMAT, byte_packing);
}

void imx233_lcdif_set_data_format(bool data_fmt_16, bool data_fmt_18, bool data_fmt_24)
{
    if(data_fmt_16)
        BF_SET(LCDIF_CTRL, DATA_FORMAT_16_BIT);
    else
        BF_CLR(LCDIF_CTRL, DATA_FORMAT_16_BIT);
    if(data_fmt_18)
        BF_SET(LCDIF_CTRL, DATA_FORMAT_18_BIT);
    else
        BF_CLR(LCDIF_CTRL, DATA_FORMAT_18_BIT);
    if(data_fmt_24)
        BF_SET(LCDIF_CTRL, DATA_FORMAT_24_BIT);
    else
        BF_CLR(LCDIF_CTRL, DATA_FORMAT_24_BIT);
}

void imx233_lcdif_wait_ready(void)
{
    while(BF_RD(LCDIF_CTRL, RUN));
}

void imx233_lcdif_pio_send(bool data_mode, unsigned len, uint32_t *buf)
{
    unsigned max_xfer_size = 0xffff;
    if(len == 0)
        return;
    if(lcdif_word_length == BV_LCDIF_CTRL_WORD_LENGTH__16_BIT)
        max_xfer_size = 0x1fffe;
    imx233_lcdif_wait_ready();
    imx233_lcdif_enable_bus_master(false);

    do
    {
        unsigned burst = MIN(len, max_xfer_size);
        len -= burst;
        unsigned count = burst;
        if(lcdif_word_length != BV_LCDIF_CTRL_WORD_LENGTH__8_BIT)
        {
            if(burst & 1)
                burst++;
            count = burst / 2;
        }
        else
            count = burst;
        HW_LCDIF_TRANSFER_COUNT = 0;
        HW_LCDIF_TRANSFER_COUNT = 0x10000 | count;
        BF_CLR(LCDIF_CTRL, DATA_SELECT);
        BF_CLR(LCDIF_CTRL, RUN);
        if(data_mode)
            BF_SET(LCDIF_CTRL, DATA_SELECT);
        BF_SET(LCDIF_CTRL, RUN);
        burst = (burst + 3) / 4;
        while(burst-- > 0)
        {
            while(BF_RD(LCDIF_STAT, LFIFO_FULL));
            HW_LCDIF_DATA = *buf++;
        }
        imx233_lcdif_wait_ready();
    }while(len > 0);
    imx233_lcdif_enable_bus_master(true);
}

void imx233_lcdif_dma_send(void *buf, unsigned width, unsigned height)
{
    HW_LCDIF_CUR_BUF = (uint32_t)buf;
    HW_LCDIF_TRANSFER_COUNT = 0;
    HW_LCDIF_TRANSFER_COUNT = (height << 16) | width;
    BF_CLR(LCDIF_CTRL, RUN);
    BF_SET(LCDIF_CTRL, DATA_SELECT);
    BF_SET(LCDIF_CTRL, RUN);
}
