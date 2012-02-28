/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C)  2011 Andrew Ryabinin
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

static void lcd_display_init(void)
{
    unsigned int x, y;

    /* Driving ability setting */
    lcd_write_reg(0x60, 0x00);
    lcd_write_reg(0x61, 0x06);
    lcd_write_reg(0x62, 0x00);
    lcd_write_reg(0x63, 0xC8);

    /* Gamma 2.2 Setting */
    lcd_write_reg(0x40, 0x00);
    lcd_write_reg(0x41, 0x40);
    lcd_write_reg(0x42, 0x45);
    lcd_write_reg(0x43, 0x01);
    lcd_write_reg(0x44, 0x60);
    lcd_write_reg(0x45, 0x05);
    lcd_write_reg(0x46, 0x0C);
    lcd_write_reg(0x47, 0xD1);
    lcd_write_reg(0x48, 0x05);

    lcd_write_reg(0x50, 0x75);
    lcd_write_reg(0x51, 0x01);
    lcd_write_reg(0x52, 0x67);
    lcd_write_reg(0x53, 0x14);
    lcd_write_reg(0x54, 0xF2);
    lcd_write_reg(0x55, 0x07);
    lcd_write_reg(0x56, 0x03);
    lcd_write_reg(0x57, 0x49);

    /* Power voltage setting */
    lcd_write_reg(0x1F, 0x03);
    lcd_write_reg(0x20, 0x00);
    lcd_write_reg(0x24, 0x28);
    lcd_write_reg(0x25, 0x45);

    lcd_write_reg(0x23, 0x2F);

    /* Power on setting */
    lcd_write_reg(0x18, 0x44);
    lcd_write_reg(0x21, 0x01);
    lcd_write_reg(0x01, 0x00);
    lcd_write_reg(0x1C, 0x03);
    lcd_write_reg(0x19, 0x06);
    udelay(5);

    /* Display on setting */
    lcd_write_reg(0x26, 0x84);
    udelay(40);
    lcd_write_reg(0x26, 0xB8);
    udelay(40);
    lcd_write_reg(0x26, 0xBC);
    udelay(40);

    /* Memmory access setting */
    lcd_write_reg(0x16, 0x48);
    /* Setup 16bit mode */
    lcd_write_reg(0x17, 0x05);

    /* Set GRAM area */
    lcd_write_reg(0x02, 0x00);
    lcd_write_reg(0x03, 0x00);
    lcd_write_reg(0x04, 0x00);
    lcd_write_reg(0x05, LCD_HEIGHT - 1);
    lcd_write_reg(0x06, 0x00);
    lcd_write_reg(0x07, 0x00);
    lcd_write_reg(0x08, 0x00);
    lcd_write_reg(0x09, LCD_WIDTH - 1);

    /* Start GRAM write */
    lcd_cmd(0x22);

    for (x=0; x<LCD_WIDTH; x++)
        for(y=0; y<LCD_HEIGHT; y++)
            lcd_data(0x00);

    display_on = true;
}

void lcd_init_device(void)
{
    lcdif_init(LCDIF_16BIT);
    lcd_display_init();
}

void lcd_enable (bool on)
{
    if (on)
    {
        lcd_write_reg(0x18, 0x44);
        lcd_write_reg(0x21, 0x01);
        lcd_write_reg(0x01, 0x00);
        lcd_write_reg(0x1C, 0x03);
        lcd_write_reg(0x19, 0x06);
        udelay(5);
        lcd_write_reg(0x26, 0x84);
        udelay(40);
        lcd_write_reg(0x26, 0xB8);
        udelay(40);
        lcd_write_reg(0x26, 0xBC);
    }
    else
    {
        lcd_write_reg(0x26, 0xB8);
        udelay(40);
        lcd_write_reg(0x19, 0x01);
        udelay(40);
        lcd_write_reg(0x26, 0xA4);
        udelay(40);
        lcd_write_reg(0x26, 0x84);
        udelay(40);
        lcd_write_reg(0x1C, 0x00);
        lcd_write_reg(0x01, 0x02);
        lcd_write_reg(0x21, 0x00);
    }
    display_on = on;

}

bool lcd_active()
{
    return display_on;
}


void lcd_update_rect(int x, int y, int width, int height)
{
    int px = x, py = y;
    int pxmax = x + width, pymax = y + height;

    lcd_write_reg(0x03, y);
    lcd_write_reg(0x05, pymax-1);
    lcd_write_reg(0x07, x);
    lcd_write_reg(0x09, pxmax-1);

    lcd_cmd(0x22);

    for (px=x; px<pxmax; px++)
	for (py=y; py<pymax; py++)
            lcd_data(*FBADDR(px, py));
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
