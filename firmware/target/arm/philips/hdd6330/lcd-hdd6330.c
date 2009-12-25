/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Mark Arigo
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
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"

/* Display status */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;

/* wait for LCD */
static inline void lcd_wait_write(void)
{
    int i = 0;
    while (LCD2_PORT & LCD2_BUSY_MASK)
    {
        if (i < 2000)
            i++;
        else
            LCD2_PORT &= ~LCD2_BUSY_MASK;
    }
}

/* send LCD data */
static void lcd_send_data(unsigned data)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_DATA_MASK | (data & 0xff);
}

/* send LCD command */
static void lcd_send_cmd(unsigned cmd)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_CMD_MASK | (cmd & 0xff);
    lcd_wait_write();
}

static inline void lcd_send_pixel(unsigned pixel)
{
    lcd_send_data(pixel >> 8);
    lcd_send_data(pixel);
}

void lcd_init_device(void)
{
    /* init handled by the OF bootloader */
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Performance function to blit a YUV bitmap directly to the LCD */
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
 
/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;
    
    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    addr = &lcd_framebuffer[y][x];

    lcd_send_cmd(0x01);
    lcd_send_data(0x48);

    lcd_send_cmd(0x05);
    lcd_send_data(0x0f);

    lcd_send_cmd(0x08);
    lcd_send_data(y);

    lcd_send_cmd(0x09);
    lcd_send_data(y + height - 1);

    lcd_send_cmd(0x0a);
    lcd_send_data(x + 16);

    lcd_send_cmd(0x0b);
    lcd_send_data(x + width - 1 + 16);

    lcd_send_cmd(0x06);
    do {
        int w = width;
        do {
            lcd_send_pixel(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
