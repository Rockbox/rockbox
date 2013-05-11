/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C)  2013 Andrew Ryabinin
 *
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
    int i;
    unsigned int x, y;

    lcd_cmd(0x00B9);
    lcd_data(0x00FF);
    lcd_data(0x0093);
    lcd_data(0x0042);

    lcd_cmd(0x0021); /* display inversion on ??? */

    /* memory access control */
    lcd_cmd(0x0036);
    lcd_data(0x00C9);

    /* set 16-bit pixel format */
    lcd_cmd(0x003A);
    lcd_data(0x0005);

    /* Setup color depth conversion lookup table */
    lcd_cmd(0x002D);
    /* red */
    for(i = 0; i < 32; i++)
        lcd_data(2*i);
    /* green */
    for(i = 0; i < 64; i++)
        lcd_data(1*i);
    /* blue */
    for(i = 0; i < 32; i++)
        lcd_data(2*i);

    /* power control settings */
    lcd_cmd(0x00C0);
    lcd_data(0x0025); /* VREG1OUT 4.70V */
    lcd_data(0x000A); /* VCOM 2.8V */

    lcd_cmd(0x00C1);
    lcd_data(0x0001);

    lcd_cmd(0x00C5);
    lcd_data(0x002F);
    lcd_data(0x0027);

    lcd_cmd(0x00C7);
    lcd_data(0x00D3);

    lcd_cmd(0x00B8);
    lcd_data(0x000B);

    /* Positive gamma correction */
    lcd_cmd(0x00E0);
    lcd_data(0x000F);
    lcd_data(0x0022);
    lcd_data(0x001D);
    lcd_data(0x000B);
    lcd_data(0x000F);
    lcd_data(0x0007);
    lcd_data(0x004C);
    lcd_data(0x0076);
    lcd_data(0x003C);
    lcd_data(0x0009);
    lcd_data(0x0016);
    lcd_data(0x0007);
    lcd_data(0x0012);
    lcd_data(0x000B);
    lcd_data(0x0008);

    /* Negative Gamma Correction */
    lcd_cmd(0x00E1);
    lcd_data(0x0008);
    lcd_data(0x001F);
    lcd_data(0x0024);
    lcd_data(0x0003);
    lcd_data(0x000E);
    lcd_data(0x0003);
    lcd_data(0x0035);
    lcd_data(0x0023);
    lcd_data(0x0045);
    lcd_data(0x0001);
    lcd_data(0x000B);
    lcd_data(0x0007);
    lcd_data(0x002F);
    lcd_data(0x0036);
    lcd_data(0x000F);

    lcd_cmd(0x00F2);
    lcd_data(0x0000);

    /* exit sleep */
    lcd_cmd(0x0011);
    udelay(5000);
    lcd_cmd(0x0029);

    lcd_cmd(0x002C);
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
    } else {
        lcd_cmd(0x10);
    }
    udelay(5000);

    display_on = on;
    LCDC_CTRL &= ~RGB24B;
}

void lcd_set_gram_area(int x_start, int y_start,
                       int x_end, int y_end)
{
    lcdctrl_bypass(1);
    LCDC_CTRL |= RGB24B;

    lcd_cmd(0x002A);
    lcd_data((x_start&0xff00)>>8);
    lcd_data(x_start&0x00ff);
    lcd_data((x_end&0xff00)>>8);
    lcd_data(x_end&0x00ff);
    lcd_cmd(0x002B);
    lcd_data((y_start&0xff00)>>8);
    lcd_data(y_start&0x00ff);
    lcd_data((y_end&0xff00)>>8);
    lcd_data(y_end&0x00ff);

    lcd_cmd(0x2c);
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
