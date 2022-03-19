
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald, Dana Conrad
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

/* for reference on these command/data hex values, see the mipi dcs lcd spec.  *
 * Not everything here is there, but all the standard stuff is.                */

static const uint32_t erosqnative_lcd_cmd_enable[] = {
    /* Set EXTC? */
    LCD_INSTR_CMD,      0xc8,
    LCD_INSTR_DAT,      0xff,
    LCD_INSTR_DAT,      0x93,
    LCD_INSTR_DAT,      0x42,
    /* Set Address Mode */
    LCD_INSTR_CMD,      0x36,
    LCD_INSTR_DAT,      0xd8,
    /* Pixel Format Set */
    LCD_INSTR_CMD,      0x3a,
    //LCD_INSTR_DAT,      0x66, /* OF specified 18 bpp */
    LCD_INSTR_DAT,      0x05, /* RB seems to be happier dealing with 16 bits/pixel */
    /* Power Control 1? */
    LCD_INSTR_CMD,      0xc0,
    LCD_INSTR_DAT,      0x15,
    LCD_INSTR_DAT,      0x15,
    /* Power Control 2? */
    LCD_INSTR_CMD,      0xc1,
    LCD_INSTR_DAT,      0x01,
    /* VCOM? */
    LCD_INSTR_CMD,      0xc5,
    LCD_INSTR_DAT,      0xda,
    /* ?? */
    LCD_INSTR_CMD,      0xb1,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x1b,
    /* ?? */
    LCD_INSTR_CMD,      0xb4,
    LCD_INSTR_DAT,      0x02,
    /* Positive gamma correction? */
    LCD_INSTR_CMD,      0xe0,
    LCD_INSTR_DAT,      0x0f,
    LCD_INSTR_DAT,      0x13,
    LCD_INSTR_DAT,      0x17,
    LCD_INSTR_DAT,      0x04,
    LCD_INSTR_DAT,      0x13,
    LCD_INSTR_DAT,      0x07,
    LCD_INSTR_DAT,      0x40,
    LCD_INSTR_DAT,      0x39,
    LCD_INSTR_DAT,      0x4f,
    LCD_INSTR_DAT,      0x06,
    LCD_INSTR_DAT,      0x0d,
    LCD_INSTR_DAT,      0x0a,
    LCD_INSTR_DAT,      0x1f,
    LCD_INSTR_DAT,      0x22,
    LCD_INSTR_DAT,      0x00,
    /* Negative gamma correction? */
    LCD_INSTR_CMD,      0xe1,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x21,
    LCD_INSTR_DAT,      0x24,
    LCD_INSTR_DAT,      0x03,
    LCD_INSTR_DAT,      0x0f,
    LCD_INSTR_DAT,      0x05,
    LCD_INSTR_DAT,      0x38,
    LCD_INSTR_DAT,      0x32,
    LCD_INSTR_DAT,      0x49,
    LCD_INSTR_DAT,      0x00,
    LCD_INSTR_DAT,      0x09,
    LCD_INSTR_DAT,      0x08,
    LCD_INSTR_DAT,      0x32,
    LCD_INSTR_DAT,      0x35,
    LCD_INSTR_DAT,      0x0f,
    /* Exit Sleep */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   120000,
    /* Display On */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_UDELAY,   20000,
    LCD_INSTR_END,
};

static const uint32_t erosqnative_lcd_of_compat_cmd[] = {
    /* Pixel Format Set */
    LCD_INSTR_CMD,      0x3a,
    LCD_INSTR_DAT,      0x66, /* 18 bpp */
    /* Exit Sleep */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   120000,
    /* Display On */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_UDELAY,   20000,
    LCD_INSTR_END,
};

/* sleep and wake copied directly from m3k */
static const uint32_t erosqnative_lcd_cmd_sleep[] = {
    /* Display OFF */
    LCD_INSTR_CMD,      0x28,
    /* Sleep IN */
    LCD_INSTR_CMD,      0x10,
    LCD_INSTR_UDELAY,   5000,
    LCD_INSTR_END,
};

static const uint32_t erosqnative_lcd_cmd_wake[] = {
    /* Sleep OUT */
    LCD_INSTR_CMD,      0x11,
    LCD_INSTR_UDELAY,   5000,
    /* Display ON */
    LCD_INSTR_CMD,      0x29,
    LCD_INSTR_END,
};

/* As far as I can tell, this is a sequence of commands sent before each
 * DMA set. Original in OF was:
 *          {0x2c, 0x2c, 0x2c, 0x2c}
 * But this set from the m3k seems to work the same, and makes more sense
 * to me:
 *          {0x00, 0x00, 0x00, 0x2c}
 * This command is more than likely going to be the same
 * for any old mipi lcd on the market, maybe. I really don't think we need
 * to send "write_memory_start four times in a row. */
static const uint8_t __attribute__((aligned(64)))
    erosqnative_lcd_dma_wr_cmd[] = {0x2c, 0x2c, 0x2c, 0x2c};

const struct lcd_tgt_config lcd_tgt_config = {
    .bus_width = 8,
    .cmd_width = 8,
    .use_6800_mode = 0,
    .use_serial = 0,
    .clk_polarity = 0,
    .dc_polarity = 0,
    .wr_polarity = 1,
    .te_enable = 0, /* OF had TE enabled (1) */
    .te_polarity = 1,
    .te_narrow = 0,
    .dma_wr_cmd_buf = &erosqnative_lcd_dma_wr_cmd,
    .dma_wr_cmd_size = sizeof(erosqnative_lcd_dma_wr_cmd),
};

void lcd_tgt_enable(bool enable)
{
    if(enable) {
        /* power up the panel */
        gpio_set_level(GPIO_LCD_PWR, 1);
        mdelay(20);
        gpio_set_level(GPIO_LCD_RESET, 1);
        mdelay(12);

        /* set the clock */
        lcd_set_clock(X1000_CLK_SCLK_A, 20000000);

        /* toggle chip select low (active) */
        gpio_set_level(GPIO_LCD_RD, 1);
        gpio_set_level(GPIO_LCD_CE, 1);
        mdelay(5);
        gpio_set_level(GPIO_LCD_CE, 0);

        lcd_exec_commands(&erosqnative_lcd_cmd_enable[0]);
    } else {
        /* doesn't flash white if we don't do anything... */
#if 0
        lcd_exec_commands(&erosqnative_lcd_cmd_sleep[0]);

        mdelay(115); // copied from m3k

        gpio_set_level(GPIO_LCD_PWR, 0);
        gpio_set_level(GPIO_LCD_RESET, 0);
#endif
    }
}

void lcd_tgt_enable_of(bool enable)
{
    /* silence the unused parameter warning */
    if (enable)
    {}

    lcd_exec_commands(&erosqnative_lcd_of_compat_cmd[0]);
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(&erosqnative_lcd_cmd_sleep[0]);
    else
        lcd_exec_commands(&erosqnative_lcd_cmd_wake[0]);
}
