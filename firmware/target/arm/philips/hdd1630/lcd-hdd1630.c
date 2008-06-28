/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#if 0
    /* this sequence from the OF bootloader */

    DEV_EN2 |= 0x2000;
    outl(inl(0x70000014) & ~0xf000000, 0x70000014);
    outl(inl(0x70000014) | 0xa000000, 0x70000014);
    DEV_INIT1 &= 0xc000;
    DEV_INIT1 |= 0x8000;
    MLCD_SCLK_DIV &= ~0x800;
    CLCD_CLOCK_SRC |= 0xc0000000;
    DEV_INIT2 &= ~0x400;
    outl(inl(0x7000002c) | ((1<<4)<<24), 0x7000002c);
    DEV_INIT2 &= ~((1<<4)<<24);
    udelay(10000);

    DEV_INIT2 |= ((1<<4)<<24);
    outl(0x220, 0x70008a00);
    outl(0x1f00, 0x70008a04);
    LCD2_BLOCK_CTRL = 0x10008080;
    LCD2_BLOCK_CONFIG = 0xF00000;

    GPIOJ_ENABLE     |= 0x4;
    GPIOJ_OUTPUT_VAL |= 0x4;
    GPIOJ_OUTPUT_EN  |= 0x4;
    
    lcd_send_cmd(0x1);
    udelay(10000);
    
    lcd_send_cmd(0x25);
    lcd_send_data(0x3f);
    
    lcd_send_cmd(0x11);
    udelay(120000);
    
    lcd_send_cmd(0x20);
    lcd_send_cmd(0x38);
    lcd_send_cmd(0x13);
    
    lcd_send_cmd(0xb4);
    lcd_send_data(0x2);
    lcd_send_data(0x6);
    lcd_send_data(0x8);
    lcd_send_data(0xd);

    lcd_send_cmd(0xb5);
    lcd_send_data(0x2);
    lcd_send_data(0x6);
    lcd_send_data(0x8);
    lcd_send_data(0xd);

    lcd_send_cmd(0xb6);
    lcd_send_data(0x19);
    lcd_send_data(0x23);
    lcd_send_data(0x2d);

    lcd_send_cmd(0xb7);
    lcd_send_data(0x5);

    lcd_send_cmd(0xba);
    lcd_send_data(0x7);
    lcd_send_data(0x18);

    lcd_send_cmd(0x36);
    lcd_send_data(0);

    lcd_send_cmd(0x3a);
    lcd_send_data(0x5);
    
    lcd_send_cmd(0x2d);
    lcd_send_data(0x1);
    lcd_send_data(0x2);
    lcd_send_data(0x3);
    lcd_send_data(0x4);
    lcd_send_data(0x5);
    lcd_send_data(0x6);
    lcd_send_data(0x7);
    lcd_send_data(0x8);
    lcd_send_data(0x9);
    lcd_send_data(0xa);
    lcd_send_data(0xb);
    lcd_send_data(0xc);
    lcd_send_data(0xd);
    lcd_send_data(0xe);
    lcd_send_data(0xf);
    lcd_send_data(0x10);
    lcd_send_data(0x11);
    lcd_send_data(0x12);
    lcd_send_data(0x13);
    lcd_send_data(0x14);
    lcd_send_data(0x15);
    lcd_send_data(0x16);
    lcd_send_data(0x17);
    lcd_send_data(0x18);
    lcd_send_data(0x19);
    lcd_send_data(0x1a);
    lcd_send_data(0x1b);
    lcd_send_data(0x1c);
    lcd_send_data(0x1d);
    lcd_send_data(0x1e);
    lcd_send_data(0x1f);
    lcd_send_data(0x20);
    lcd_send_data(0x21);
    lcd_send_data(0x22);
    lcd_send_data(0x23);
    lcd_send_data(0x24);
    lcd_send_data(0x25);
    lcd_send_data(0x26);
    lcd_send_data(0x27);
    lcd_send_data(0x28);
    lcd_send_data(0x29);
    lcd_send_data(0x2a);
    lcd_send_data(0x2b);
    lcd_send_data(0x2c);
    lcd_send_data(0x2d);
    lcd_send_data(0x2e);
    lcd_send_data(0x2f);
    lcd_send_data(0x30);

    lcd_send_cmd(0x29);
#endif
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

    lcd_send_cmd(0x2a);
    lcd_send_data(x);
    lcd_send_data(x + width - 1);

    lcd_send_cmd(0x2b);
    lcd_send_data(y);
    lcd_send_data(y + height - 1);

    lcd_send_cmd(0x2c);
    do {
        int w = width;
        do {
            lcd_send_pixel(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
