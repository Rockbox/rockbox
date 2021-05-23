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

#include "lcd.h"
#include "system.h"
#include "lcd-x1000.h"
#include "gpio-x1000.h"

/* LCD controller is probably an RM68090.
 */

static const uint32_t q1_lcd_cmd_enable[] = {
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0xbe,
    LCD_INSTR_DAT, 0xc3,
    LCD_INSTR_DAT, 0x29,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x04,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x10,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x05,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x06,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x07,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x08,
    LCD_INSTR_DAT, 0x03,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x0d,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x10,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0xc1,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x11,
    LCD_INSTR_DAT, 0xb1,
    LCD_INSTR_DAT, 0x08,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x12,
    LCD_INSTR_DAT, 0xb1,
    LCD_INSTR_DAT, 0x08,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x13,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x0f,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x14,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x14,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x15,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x04,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x16,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x22,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x23,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x30,
    LCD_INSTR_DAT, 0x7c,
    LCD_INSTR_DAT, 0x3f,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x32,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x70,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x91,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0xe0,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0xe1,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x61,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_DAT, 0x10,
    LCD_INSTR_DAT, 0x30,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_DAT, 0xf6,
    LCD_INSTR_DAT, 0x3f,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_DAT, 0x50,
    LCD_INSTR_DAT, 0x1f,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x30,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x08,
    LCD_INSTR_DAT, 0x03,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x11,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x35,
    LCD_INSTR_DAT, 0x76,
    LCD_INSTR_DAT, 0x66,

    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_CMD, 0x39,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x26,

    LCD_INSTR_CMD, 0x04,
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0xc7,

    LCD_INSTR_CMD, 0x04,
    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x06,
    LCD_INSTR_CMD, 0x06,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_DAT, 0x0d,
    LCD_INSTR_DAT, 0x0e,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_DAT, 0x08,
    LCD_INSTR_DAT, 0x08,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_DAT, 0x02,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x04,
    LCD_INSTR_DAT, 0x03,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x05,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x04,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x06,
    LCD_INSTR_DAT, 0x1b,
    LCD_INSTR_DAT, 0x21,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x07,
    LCD_INSTR_DAT, 0x0f,
    LCD_INSTR_DAT, 0x0e,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x08,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x04,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x09,
    LCD_INSTR_DAT, 0x08,
    LCD_INSTR_DAT, 0x08,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x0a,
    LCD_INSTR_DAT, 0x02,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x0b,
    LCD_INSTR_DAT, 0x03,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x0c,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_CMD, 0x0d,
    LCD_INSTR_DAT, 0x31,
    LCD_INSTR_DAT, 0x34,

    /* X start */
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x10,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x1e, /* 30 */

    /* X end */
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x11,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x85, /* 389 */

    /* Y start */
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x12,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00, /* 0 */

    /* Y end */
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x13,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x8f, /* 399 */

    /* RAM write start X? */
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x1e,

    /* RAM write start Y? */
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x01,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x03,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x30,

    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_CMD, 0x02,
    LCD_INSTR_END,
};

/* NOTE this sleep mode may not be saving power, but it gets rid of the
 * ghost image that would otherwise remain on the display */
static const uint32_t q1_lcd_cmd_sleep[] = {
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x10,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x07,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x01,

    LCD_INSTR_END,
};

static const uint32_t q1_lcd_cmd_wake[] = {
    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x07,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0x00,
    LCD_INSTR_CMD, 0x10,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0xc1,

    LCD_INSTR_END,
};

static const uint8_t __attribute__((aligned(64)))
    q1_lcd_dma_wr_cmd[] = {0x02, 0x02, 0x02, 0x02};

const struct lcd_tgt_config lcd_tgt_config = {
    .bus_width = 8,
    .cmd_width = 8,
    .use_6800_mode = 0,
    .use_serial = 0,
    .clk_polarity = 0,
    .dc_polarity = 0,
    .wr_polarity = 1,
    .te_enable = 0,
    .big_endian = 1,
    .dma_wr_cmd_buf = &q1_lcd_dma_wr_cmd,
    .dma_wr_cmd_size = sizeof(q1_lcd_dma_wr_cmd),
};

void lcd_tgt_enable(bool enable)
{
    if(enable) {
        /* power on the panel */
        gpio_set_level(GPIO_LCD_PWR, 1);
        gpio_set_level(GPIO_LCD_RST, 1);
        gpio_set_level(GPIO_LCD_CE, 1);
        gpio_set_level(GPIO_LCD_RD, 1);
        mdelay(50);
        gpio_set_level(GPIO_LCD_RST, 0);
        mdelay(100);
        gpio_set_level(GPIO_LCD_RST, 1);
        mdelay(50);
        gpio_set_level(GPIO_LCD_CE, 0);

        /* Start the controller */
        lcd_set_clock(X1000_CLK_MPLL, 50000000);
        lcd_exec_commands(q1_lcd_cmd_enable);
    } else {
        /* FIXME: Shanling Q1 LCD power down sequence
         * not important because we don't use it but it'd be nice to know */
    }
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(q1_lcd_cmd_sleep);
    else
        lcd_exec_commands(q1_lcd_cmd_wake);
}
