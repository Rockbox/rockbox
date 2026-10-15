/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Marcin Bukat
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
#include "gpio-ingenic.h"

/* Sitronix ST7735S, 128x160, 8-bit 8080 bus (PA0-7, WR=PB17, DC=PB20).
 * CS (PB18) and RD (PB16) are plain GPIOs set up by the OF U-Boot, the
 * OF kernel never touches them. TE is not connected.
 *
 * Init sequence taken from the OF kernel (jzfb platform data), except
 * COLMOD is set to RGB565 instead of RGB666 and the column/row window
 * is set explicitly. */

#define ST7735S_DELAY(ms) LCD_INSTR_UDELAY, ((ms) * 1000)

static const uint32_t rocker_lcd_cmd_enable[] = {
    LCD_INSTR_CMD, 0x11,        /* sleep out */
    ST7735S_DELAY(120),

    LCD_INSTR_CMD, 0xb1,        /* frame rate control (normal mode) */
    LCD_INSTR_DAT, 0x05,
    LCD_INSTR_DAT, 0x3c,
    LCD_INSTR_DAT, 0x3c,

    LCD_INSTR_CMD, 0xb2,        /* frame rate control (idle mode) */
    LCD_INSTR_DAT, 0x05,
    LCD_INSTR_DAT, 0x3c,
    LCD_INSTR_DAT, 0x3c,

    LCD_INSTR_CMD, 0xb3,        /* frame rate control (partial mode) */
    LCD_INSTR_DAT, 0x05,
    LCD_INSTR_DAT, 0x3c,
    LCD_INSTR_DAT, 0x3c,
    LCD_INSTR_DAT, 0x05,
    LCD_INSTR_DAT, 0x3c,
    LCD_INSTR_DAT, 0x3c,

    LCD_INSTR_CMD, 0xb4,        /* display inversion control */
    LCD_INSTR_DAT, 0x03,

    LCD_INSTR_CMD, 0xc0,        /* power control 1 */
    LCD_INSTR_DAT, 0x28,
    LCD_INSTR_DAT, 0x08,
    LCD_INSTR_DAT, 0x04,

    LCD_INSTR_CMD, 0xc1,        /* power control 2 */
    LCD_INSTR_DAT, 0xc0,

    LCD_INSTR_CMD, 0xc2,        /* power control 3 */
    LCD_INSTR_DAT, 0x0d,
    LCD_INSTR_DAT, 0x00,

    LCD_INSTR_CMD, 0xc3,        /* power control 4 */
    LCD_INSTR_DAT, 0x8d,
    LCD_INSTR_DAT, 0x2a,

    LCD_INSTR_CMD, 0xc4,        /* power control 5 */
    LCD_INSTR_DAT, 0x8d,
    LCD_INSTR_DAT, 0xee,

    LCD_INSTR_CMD, 0xc5,        /* VCOM control */
    LCD_INSTR_DAT, 0x1a,

    LCD_INSTR_CMD, 0x36,        /* MADCTL: MY | MX | BGR */
    LCD_INSTR_DAT, 0xc8,

    LCD_INSTR_CMD, 0xe0,        /* positive gamma */
    LCD_INSTR_DAT, 0x04,
    LCD_INSTR_DAT, 0x22,
    LCD_INSTR_DAT, 0x07,
    LCD_INSTR_DAT, 0x0a,
    LCD_INSTR_DAT, 0x2e,
    LCD_INSTR_DAT, 0x30,
    LCD_INSTR_DAT, 0x25,
    LCD_INSTR_DAT, 0x2a,
    LCD_INSTR_DAT, 0x28,
    LCD_INSTR_DAT, 0x26,
    LCD_INSTR_DAT, 0x2e,
    LCD_INSTR_DAT, 0x3a,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x03,
    LCD_INSTR_DAT, 0x13,

    LCD_INSTR_CMD, 0xe1,        /* negative gamma */
    LCD_INSTR_DAT, 0x04,
    LCD_INSTR_DAT, 0x16,
    LCD_INSTR_DAT, 0x06,
    LCD_INSTR_DAT, 0x0d,
    LCD_INSTR_DAT, 0x2d,
    LCD_INSTR_DAT, 0x26,
    LCD_INSTR_DAT, 0x23,
    LCD_INSTR_DAT, 0x27,
    LCD_INSTR_DAT, 0x27,
    LCD_INSTR_DAT, 0x25,
    LCD_INSTR_DAT, 0x2d,
    LCD_INSTR_DAT, 0x3b,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x01,
    LCD_INSTR_DAT, 0x04,
    LCD_INSTR_DAT, 0x13,

    LCD_INSTR_CMD, 0x3a,        /* COLMOD: 16 bpp (OF uses 0x06, 18 bpp) */
    LCD_INSTR_DAT, 0x05,

    LCD_INSTR_CMD, 0x2a,        /* CASET 0..127 */
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, LCD_WIDTH - 1,

    LCD_INSTR_CMD, 0x2b,        /* RASET 0..159 */
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, 0x00,
    LCD_INSTR_DAT, LCD_HEIGHT - 1,

    LCD_INSTR_CMD, 0x29,        /* display on */
    LCD_INSTR_END,
};

static const uint32_t rocker_lcd_cmd_sleep[] = {
    LCD_INSTR_CMD, 0x28,        /* display off */
    LCD_INSTR_CMD, 0x10,        /* sleep in */
    ST7735S_DELAY(5),
    LCD_INSTR_END,
};

static const uint32_t rocker_lcd_cmd_wake[] = {
    LCD_INSTR_CMD, 0x11,        /* sleep out */
    ST7735S_DELAY(120),
    LCD_INSTR_CMD, 0x29,        /* display on */
    LCD_INSTR_END,
};

static const uint8_t __attribute__((aligned(64)))
    rocker_lcd_dma_wr_cmd[] = {0x2c, 0x2c, 0x2c, 0x2c};

const struct lcd_tgt_config lcd_tgt_config = {
    .bus_width = 8,
    .cmd_width = 8,
    .use_6800_mode = 0,
    .use_serial = 0,
    .clk_polarity = 0,
    .dc_polarity = 0,
    .wr_polarity = 1,
    .te_enable = 0,
    .big_endian = 0,
    .dma_wr_cmd_buf = &rocker_lcd_dma_wr_cmd,
    .dma_wr_cmd_size = sizeof(rocker_lcd_dma_wr_cmd),
};

void lcd_tgt_enable(bool enable)
{
    if(enable) {
        /* Same sequence as the OF U-Boot st7735s_128160 driver */
        gpio_set_level(GPIO_LCD_PWR, 1);
        mdelay(20);
        gpio_set_level(GPIO_LCD_CE, 1);
        gpio_set_level(GPIO_LCD_RD, 1);
        gpio_set_level(GPIO_LCD_RST, 0);
        mdelay(20);
        gpio_set_level(GPIO_LCD_RST, 1);
        mdelay(10);
        gpio_set_level(GPIO_LCD_CE, 0);

        /* ST7735S needs >= 66 ns write cycle (~15 MHz), the SLCD strobes WR
         * once per pixel clock. The OF's 30 MHz garbles DMA transfers. */
        lcd_set_clock(X1000_CLK_MPLL, 10000000);
        lcd_exec_commands(rocker_lcd_cmd_enable);
    } else {
        lcd_exec_commands(rocker_lcd_cmd_sleep);
        gpio_set_level(GPIO_LCD_CE, 1);
        gpio_set_level(GPIO_LCD_RST, 0);
        gpio_set_level(GPIO_LCD_PWR, 0);
    }
}

void lcd_tgt_sleep(bool sleep)
{
    if(sleep)
        lcd_exec_commands(rocker_lcd_cmd_sleep);
    else
        lcd_exec_commands(rocker_lcd_cmd_wake);
}
