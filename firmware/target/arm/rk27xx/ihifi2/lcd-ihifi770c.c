/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

#include "config.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "lcdif-rk27xx.h"

static bool display_on = false;

void lcd_display_init(void)
{
    unsigned int x, y;

    lcd_cmd(0x13);

    mdelay(120);

    lcd_cmd(0x35);
    lcd_data(0x00);

    lcd_cmd(0x36);
    lcd_data(0x48);

    lcd_cmd(0xD0);
    lcd_data(0x00);
    lcd_data(0x05);

    lcd_cmd(0xEF);
    lcd_data(0x07);

    lcd_cmd(0xF2);
    lcd_data(0x1B);
    lcd_data(0x16);
    lcd_data(0x0F);
    lcd_data(0x08);
    lcd_data(0x08);
    lcd_data(0x08);
    lcd_data(0x08);
    lcd_data(0x10);
    lcd_data(0x00);
    lcd_data(0x1C);
    lcd_data(0x16);

    lcd_cmd(0xF3);
    lcd_data(0x01);
    lcd_data(0x41);
    lcd_data(0x15);
    lcd_data(0x0D);
    lcd_data(0x33);
    lcd_data(0x63);
    lcd_data(0x46);
    lcd_data(0x10);

    lcd_cmd(0xF4);
    lcd_data(0x5B);
    lcd_data(0x5B);
    lcd_data(0x55);
    lcd_data(0x55);
    lcd_data(0x44);

    lcd_cmd(0xF5);
    lcd_data(0x12);
    lcd_data(0x11);
    lcd_data(0x06);
    lcd_data(0xF0);
    lcd_data(0x00);
    lcd_data(0x1F);

    lcd_cmd(0xF6);
    lcd_data(0x80);
    lcd_data(0x10);
    lcd_data(0x00);

    lcd_cmd(0xFD);
    lcd_data(0x11);
    lcd_data(0x1D);
    lcd_data(0x00);

    lcd_cmd(0xF8);
    lcd_data(0x00);
    lcd_data(0x15);
    lcd_data(0x01);
    lcd_data(0x08);
    lcd_data(0x15);
    lcd_data(0x22);
    lcd_data(0x25);
    lcd_data(0x28);
    lcd_data(0x14);
    lcd_data(0x13);
    lcd_data(0x10);
    lcd_data(0x11);
    lcd_data(0x09);
    lcd_data(0x24);
    lcd_data(0x28);

    lcd_cmd(0xF9);
    lcd_data(0x00);
    lcd_data(0x15);
    lcd_data(0x01);
    lcd_data(0x08);
    lcd_data(0x15);
    lcd_data(0x22);
    lcd_data(0x25);
    lcd_data(0x28);
    lcd_data(0x14);
    lcd_data(0x13);
    lcd_data(0x10);
    lcd_data(0x11);
    lcd_data(0x09);
    lcd_data(0x24);
    lcd_data(0x28);

    lcd_cmd(0xFC);
    lcd_data(0x00);
    lcd_data(0x15);
    lcd_data(0x01);
    lcd_data(0x08);
    lcd_data(0x15);
    lcd_data(0x22);
    lcd_data(0x25);
    lcd_data(0x28);
    lcd_data(0x14);
    lcd_data(0x13);
    lcd_data(0x10);
    lcd_data(0x11);
    lcd_data(0x09);
    lcd_data(0x24);
    lcd_data(0x28);

    lcd_cmd(0x36);
    lcd_data(0x48);

    lcd_cmd(0x3A);
    lcd_data(0x55);

    lcd_cmd(0x2A);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data(0x01);
    lcd_data(0x3F);

    lcd_cmd(0x2B);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data(0xEF);

    lcd_cmd(0x11);

    mdelay(120);

    lcd_cmd(0x29);

    lcd_cmd(0x2C);

    for (x = 0; x < LCD_WIDTH; x++)
        for(y=0; y < LCD_HEIGHT; y++)
            lcd_data(0x00);

    display_on = true;
}

void lcd_enable (bool on)
{
    if (on == display_on)
        return;

    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    if (on) {
        lcd_cmd(0x11);
        mdelay(120);
        lcd_cmd(0x29);
        lcd_cmd(0x2C);
    } else {
        lcd_cmd(0x28);
        mdelay(120);
        lcd_cmd(0x10);
    }

    display_on = on;
    LCDC_CTRL &= ~RGB24B;
}

void lcd_set_gram_area(int x_start, int y_start,
                       int x_end, int y_end)
{
    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    lcd_cmd(0x2A);
    lcd_data((x_start&0xff00)>>8);
    lcd_data(x_start&0x00ff);
    lcd_data((x_end&0xff00)>>8);
    lcd_data(x_end&0x00ff);

    lcd_cmd(0x2B);
    lcd_data((y_start&0xff00)>>8);
    lcd_data(y_start&0x00ff);
    lcd_data((y_end&0xff00)>>8);
    lcd_data(y_end&0x00ff);

    lcd_cmd(0x2C);

    LCDC_CTRL &= ~RGB24B;
}

bool lcd_active()
{
    return display_on;
}

/* Blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    (void)src;
    (void)src_x;
    (void)src_y;
    (void)stride;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
