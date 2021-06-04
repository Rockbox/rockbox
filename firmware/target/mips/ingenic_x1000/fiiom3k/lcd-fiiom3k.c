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
#include "kernel.h"
#include "lcd-x1000.h"
#include "gpio-x1000.h"
#include "system.h"

static const uint32_t fiio_lcd_cmd_enable[] = {
    /* Software reset */
    LCD_INSTR_CMD,      0x01,
    LCD_INSTR_UDELAY,   120000,
    /* Sleep out */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   5000,
    /* Memory access order */
    LCD_INSTR_CMD,      0x36,
    LCD_INSTR_DAT,      0x00,
    /* Row and column address set */
    LCD_INSTR_CMD,      0x2a,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      (LCD_WIDTH >> 8) & 0xff,
    LCD_INSTR_DAT,      (LCD_WIDTH & 0xff),
    LCD_INSTR_CMD,      0x2b,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      (LCD_HEIGHT >> 8) & 0xff,
    LCD_INSTR_DAT,      (LCD_HEIGHT & 0xff),
    /* Interface pixel format */
    LCD_INSTR_CMD,      0x3a,
    LCD_INSTR_DAT,      0x05,
    /* Enable display inversion */
    LCD_INSTR_CMD,      0x21,
    /* Porch setting */
    LCD_INSTR_CMD,      0xb2,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x33,
    /* Gate control */
    LCD_INSTR_CMD,      0xb7,
    LCD_INSTR_DAT,      0x35,
    /* VCOM setting */
    LCD_INSTR_CMD,      0xbb,
    LCD_INSTR_DAT,      0x1f,
    /* Backlight control 5 */
    LCD_INSTR_CMD,      0xbc,
    LCD_INSTR_DAT,      0xec,
    /* Backlight control 6 */
    LCD_INSTR_CMD,      0xbd,
    LCD_INSTR_DAT,      0xfe,
    /* Voltage settings */
    LCD_INSTR_CMD,      0xc2,
    LCD_INSTR_DAT,      0x01,
    LCD_INSTR_CMD,      0xc3,
    LCD_INSTR_DAT,      0x19,
    LCD_INSTR_CMD,      0xc4,
    LCD_INSTR_DAT,      0x20,
    /* Frame rate control */
    LCD_INSTR_CMD,      0xc6,
    LCD_INSTR_DAT,      0x0f, /* = 60 fps */
    /* Power control 1 */
    LCD_INSTR_CMD,      0xd0,
    LCD_INSTR_DAT,      0xa4,
    LCD_INSTR_DAT,      0xa1,
    /* d6 Unknown */
    LCD_INSTR_CMD,      0xd6,
    LCD_INSTR_DAT,      0xa1,
    /* Positive gamma correction */
    LCD_INSTR_CMD,      0xe0,
    LCD_INSTR_DAT,      0xd0,
    LCD_INSTR_DAT,      0x06,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x09,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x32,
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x49,
    LCD_INSTR_DAT,      0x19,
    LCD_INSTR_DAT,      0x14,
    LCD_INSTR_DAT,      0x15,
    LCD_INSTR_DAT,      0x2b,
    LCD_INSTR_DAT,      0x34,
    /* Negative gamma correction */
    LCD_INSTR_CMD,      0xe1,
    LCD_INSTR_DAT,      0xd0,
    LCD_INSTR_DAT,      0x06,
    LCD_INSTR_DAT,      0x0c,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x09,
    LCD_INSTR_DAT,      0x11,
    LCD_INSTR_DAT,      0x37,
    LCD_INSTR_DAT,      0x33,
    LCD_INSTR_DAT,      0x49,
    LCD_INSTR_DAT,      0x19,
    LCD_INSTR_DAT,      0x14,
    LCD_INSTR_DAT,      0x15,
    LCD_INSTR_DAT,      0x2d,
    LCD_INSTR_DAT,      0x34,
    /* Tearing effect line ON, mode=0 (vsync signal) */
    LCD_INSTR_CMD,      0x35,
    LCD_INSTR_DAT,      0x00,
    /* Display ON */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_END,
};

static const uint32_t fiio_lcd_cmd_sleep[] = {
    /* Display OFF */
    LCD_INSTR_CMD,      0x28,
    /* Sleep IN */
    LCD_INSTR_CMD,      0x10,
    LCD_INSTR_UDELAY,   5000,
    LCD_INSTR_END,
};

static const uint32_t fiio_lcd_cmd_wake[] = {
    /* Sleep OUT */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   5000,
    /* Display ON */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_END,
};

static const uint8_t __attribute__((aligned(64)))
    fiio_lcd_dma_wr_cmd[] = {0x00, 0x00, 0x00, 0x2c};

const struct lcd_tgt_config lcd_tgt_config = {
    .bus_width = 16,
    .cmd_width = 8,
    .use_6800_mode = 0,
    .use_serial = 0,
    .clk_polarity = 0,
    .dc_polarity = 0,
    .wr_polarity = 1,
    .te_enable = 1,
    .te_polarity = 1,
    .te_narrow = 0,
    .dma_wr_cmd_buf = &fiio_lcd_dma_wr_cmd,
    .dma_wr_cmd_size = sizeof(fiio_lcd_dma_wr_cmd),
};

void lcd_tgt_enable(bool enable)
{
    if(enable) {
        /* reset controller, probably */
        gpio_set_level(GPIO_LCD_CE, 1);
        gpio_set_level(GPIO_LCD_RD, 1);
        mdelay(5);
        gpio_set_level(GPIO_LCD_CE, 0);

        /* set the clock whatever it is... */
        lcd_set_clock(X1000_CLK_SCLK_A, 30000000);

        /* program the initial configuration */
        lcd_exec_commands(&fiio_lcd_cmd_enable[0]);
    } else {
        /* go to sleep mode first */
        lcd_exec_commands(&fiio_lcd_cmd_sleep[0]);

        /* ensure we wait a total of 120ms before power off */
        mdelay(115);

        /* this is intended to power off the panel but I'm not sure it does */
        gpio_set_level(GPIO_LCD_CE, 0);
        gpio_set_level(GPIO_LCD_RD, 0);
    }
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(&fiio_lcd_cmd_sleep[0]);
    else
        lcd_exec_commands(&fiio_lcd_cmd_wake[0]);
}
