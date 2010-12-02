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
    while (LCD2_PORT & LCD2_BUSY_MASK);
}

/* send LCD data */
static void lcd_send_data(unsigned data)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_DATA_MASK | (data & 0xff);
}

/* send LCD command */
static void lcd_send_reg(unsigned reg)
{
    lcd_wait_write();
    LCD2_PORT = LCD2_CMD_MASK | (reg & 0xff);
    lcd_wait_write();
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
    unsigned long *addr;
    int new_x, new_width;

    /* Ensure x and width are both even - so we can read 32-bit aligned 
       data from lcd_framebuffer */
    new_x = x&~1;
    new_width = width&~1;
    if (new_x+new_width < x+width) new_width += 2;
    x = new_x;
    width = new_width;

    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    lcd_send_reg(0x01);
    lcd_send_data(0x48);

    lcd_send_reg(0x05);
    lcd_send_data(0x0f);

    lcd_send_reg(0x08);
    lcd_send_data(y);

    lcd_send_reg(0x09);
    lcd_send_data(y + height - 1);

    lcd_send_reg(0x0a);
    lcd_send_data(x + 16);

    lcd_send_reg(0x0b);
    lcd_send_data(x + width - 1 + 16);

    lcd_send_reg(0x06);

    addr = (unsigned long*)&lcd_framebuffer[y][x];

    while (height > 0) 
    {
        int c, r;
        int h, pixels_to_write;

        pixels_to_write = (width * height) * 2;
        h = height;

        /* calculate how much we can do in one go */
        if (pixels_to_write > 0x10000) 
        {
            h = (0x10000/2) / width;
            pixels_to_write = (width * h) * 2;
        }

        LCD2_BLOCK_CTRL = 0x10000080;
        LCD2_BLOCK_CONFIG = 0xc0010000 | (pixels_to_write - 1);
        LCD2_BLOCK_CTRL = 0x34000000;

        /* for each row */
        for (r = 0; r < h; r++) 
        {
            /* for each column */
            for (c = 0; c < width; c += 2) 
            {
                while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_TXOK));

                /* output 2 pixels */
                LCD2_BLOCK_DATA = *addr++;
            }
            addr += (LCD_WIDTH - width)/2;
        }

        while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_READY));
        LCD2_BLOCK_CONFIG = 0;

        height -= h;
    }
}
