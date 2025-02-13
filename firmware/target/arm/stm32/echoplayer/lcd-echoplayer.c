/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "spi-stm32h7.h"
#include "gpio-stm32h7.h"
#include "stm32h7/rcc.h"

struct stm_spi_config spi_cfg = {
    .num = STM_SPI5,
    .mode = STM_SPIMODE_HALF_DUPLEX,
    .proto = STM_SPIPROTO_MOTOROLA,
    .frame_bits = 9,
    .hw_cs_output = true,
};

struct stm_spi spi;

#define ili_cmd(cmd, ...) \
    do { \
        uint16_t arr[] = {cmd, __VA_ARGS__}; \
        for (size_t i = 1; i < ARRAYLEN(arr); ++i) \
            arr[i] |= 0x100; \
        stm_spi_transmit(&spi, arr, sizeof(arr)); \
    } while (0)

static void set_row_column_address(int x, int y, int w, int h)
{
    ili_cmd(0x2a, x >> 8, x & 0xff, (w-1) >> 8, (w-1) & 0xff);
    ili_cmd(0x2b, y >> 8, y & 0xff, (h-1) >> 8, (h-1) & 0xff);
}

void lcd_init_device(void)
{
    /* Clock configuration -- should be 12 MHz (SPI clock is 1/2 of HSE) */
    st_writef(RCC_D2CCIP1R, SPI45SEL_V(HSE));
    st_writef(RCC_APB2ENR, SPI5EN(1));

    /* Configure SPI bus */
    stm_spi_init(&spi, &spi_cfg);

    /* Ensure controller is reset */
    gpio_set_level(GPIO_LCD_RESET, 0);
    sleep(12);

    gpio_set_level(GPIO_LCD_RESET, 1);
    sleep(12);

    /* Sleep out */
    ili_cmd(0x11);
    sleep(12);

    /* memory access control (X/Y invert, BGR panel) */
    ili_cmd(0x36, 0xc8);

    /* pixel format set (16bpp) */
    ili_cmd(0x3a, 0x55);

    /* display ON */
    ili_cmd(0x29);
}

bool lcd_active(void)
{
    return true;
}

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x, int y, int width, int height)
{
    /* row buffer to minimize time wasted in post-transaction delay */
    static uint16_t row[LCD_WIDTH * 2];

    if (x < 0)
        x = 0;
    else if (x >= LCD_WIDTH)
        x = LCD_WIDTH - 1;

    if (y < 0)
        y = 0;
    else if (y >= LCD_HEIGHT)
        y = LCD_HEIGHT - 1;

    if (width > LCD_WIDTH - x)
        width = LCD_WIDTH - x;

    if (height > LCD_HEIGHT - y)
        height = LCD_HEIGHT - y;

    set_row_column_address(x, y, width, height);
    ili_cmd(0x2c);

    for (int py = y; py < height; ++py)
    {
        for (int px = x; px < width; ++px)
        {
            fb_data *fb = FBADDR(px, py);
            uint16_t *data = &row[px * 2];

            data[0] = 0x100;
            data[0] |= (FB_UNPACK_RED(*fb) >> 3) << 3;
            data[0] |= (FB_UNPACK_GREEN(*fb) >> 5);

            data[1] = 0x100;
            data[1] |= ((FB_UNPACK_GREEN(*fb) >> 2) & 0x7) << 5;
            data[1] |= (FB_UNPACK_BLUE(*fb) >> 3);
        }

        stm_spi_transmit(&spi, &row[x * 2], width * sizeof(*row) * 2);
    }
}
