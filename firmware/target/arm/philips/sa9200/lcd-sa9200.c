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
    while (LCD1_CONTROL & LCD1_BUSY_MASK);
}

/* send LCD data */
static void lcd_send_data(unsigned data)
{
    lcd_wait_write();
    LCD1_DATA = data >> 8;
    lcd_wait_write();
    LCD1_DATA = data & 0xff;
}

/* send LCD command */
static void lcd_send_command(unsigned cmd)
{
    lcd_wait_write();
    LCD1_CMD = cmd >> 8;
    lcd_wait_write();
    LCD1_CMD = cmd & 0xff;
}

static void lcd_write_reg(unsigned reg, unsigned data)
{
    lcd_send_command(reg);
    lcd_send_data(data);
}

void lcd_init_device(void)
{
#if 0
    /* This is the init done by the OF bootloader.
       Re-initializing the lcd causes it to flash
       a white screen, so for now disable this. */
    DEV_INIT1 &= ~0x3000;
    DEV_INIT1 = DEV_INIT1;
    DEV_INIT2 &= ~0x400;

    LCD1_CONTROL = 0x4680;
    udelay(1500);
    LCD1_CONTROL = 0x4684;

    outl(1, 0x70003018);

    LCD1_CONTROL &= ~0x200;
    LCD1_CONTROL &= ~0x800;
    LCD1_CONTROL &= ~0x400;
    udelay(30000);

    LCD1_CONTROL |= 0x1;

    lcd_write_reg(0x0000, 0x0001);
    udelay(50000);

    lcd_write_reg(0x0011, 0x171f);
    lcd_write_reg(0x0012, 0x0001);
    lcd_write_reg(0x0013, 0x08cd);
    lcd_write_reg(0x0014, 0x0416);
    lcd_write_reg(0x0010, 0x1208);
    udelay(50000);

    lcd_write_reg(0x0013, 0x081C);
    udelay(200000);

    lcd_write_reg(0x0001, 0x0a0c);
    lcd_write_reg(0x0002, 0x0200);
    lcd_write_reg(0x0003, 0x1030);
    lcd_write_reg(0x0007, 0x0005);
    lcd_write_reg(0x0008, 0x030a);
    lcd_write_reg(0x000b, 0x0000);
    lcd_write_reg(0x000c, 0x0000);
    lcd_write_reg(0x0030, 0x0000);
    lcd_write_reg(0x0031, 0x0204);
    lcd_write_reg(0x0032, 0x0001);
    lcd_write_reg(0x0033, 0x0600);
    lcd_write_reg(0x0034, 0x0607);
    lcd_write_reg(0x0035, 0x0305);
    lcd_write_reg(0x0036, 0x0707);
    lcd_write_reg(0x0037, 0x0006);
    lcd_write_reg(0x0038, 0x0400);
    lcd_write_reg(0x0040, 0x0000);
    lcd_write_reg(0x0042, 0x9f00);
    lcd_write_reg(0x0043, 0x0000);
    lcd_write_reg(0x0044, 0x7f00);
    lcd_write_reg(0x0045, 0x9f00);
    lcd_write_reg(0x00a8, 0x0125);
    lcd_write_reg(0x00a9, 0x0014);
    lcd_write_reg(0x00a7, 0x0022);

    lcd_write_reg(0x0007, 0x0021);
    udelay(40000);
    lcd_write_reg(0x0007, 0x0023);
    udelay(40000);
    lcd_write_reg(0x0007, 0x1037);

    lcd_write_reg(0x0021, 0x0000);
#endif
}

/*** hardware configuration ***/
#if 0
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}
#endif

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

    do {
        lcd_write_reg(0x0021, ((y++ & 0xff) << 8) | (x & 0xff));
        lcd_send_command(0x0022);

        int w = width;
        do {
            lcd_send_data(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
