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

/* register defines for Philips LCD 220x176x16 - model: LPH9165-2 */
#define LCD_REG_UNKNOWN_00        0x00
#define LCD_REG_UNKNOWN_01        0x01
#define LCD_REG_UNKNOWN_05        0x05
#define LCD_REG_WRITE_DATA_2_GRAM 0x06
#define LCD_REG_HORIZ_ADDR_START  0x08
#define LCD_REG_HORIZ_ADDR_END    0x09
#define LCD_REG_VERT_ADDR_START   0x0a
#define LCD_REG_VERT_ADDR_END     0x0b

/* whether the lcd is currently enabled or not */
static bool lcd_enabled;

/* Display status */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;

/* Value used for flipping. Must be remembered when display is turned off. */
static unsigned short flip;

/* Used for flip offset correction */
static int x_offset;

/* Inverse value. Must be remembered when display is turned off. */
static unsigned short invert;

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
    x_offset = 16;
    invert = 0x00;
    flip   = 0x40;

    lcd_enabled = true;
}

#ifdef HAVE_LCD_ENABLE
/* enable / disable lcd */
void lcd_enable(bool on)
{
    if (on == lcd_enabled)
        return;

    if (on) /* lcd_display_on() */
    {
        lcd_send_reg(LCD_REG_UNKNOWN_00);
        lcd_send_data(0x00 | invert);
        lcd_send_reg(LCD_REG_UNKNOWN_01);
        lcd_send_data(0x08 | flip);
        lcd_send_reg(LCD_REG_UNKNOWN_05);
        lcd_send_data(0x0f);
        sleep(HZ/10); /* 100ms */

        /* Probably out of sync and we don't wanna pepper the code with
           lcd_update() calls for this. */
        lcd_update();
        send_event(LCD_EVENT_ACTIVATION, NULL);

        lcd_enabled = true;
    }
    else /* lcd_display_off() */
    {
        lcd_send_reg(LCD_REG_UNKNOWN_00);
        lcd_send_data(0x08);

        lcd_enabled = false;
    }
}


bool lcd_active(void)
{
    return lcd_enabled;
}
#endif /* HAVE_LCD_ENABLE */


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
    invert = (yesno) ? 0x40 : 0x00;
    lcd_send_reg(LCD_REG_UNKNOWN_00);
    lcd_send_data(invert);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    flip = (yesno) ? 0x80 : 0x40;
    x_offset = (yesno) ? 4 : 16;
    lcd_send_reg(LCD_REG_UNKNOWN_01);
    lcd_send_data(0x08 | flip);
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

#define CSUB_X 2
#define CSUB_Y 2

/*   YUV- > RGB565 conversion
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
*/

extern void lcd_yuv_write_inner_loop(unsigned char const * const ysrc,
                                     unsigned char const * const usrc,
                                     unsigned char const * const vsrc,
                                     int width);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    int h;

    width = (width + 1) & ~1;

    lcd_send_reg(LCD_REG_HORIZ_ADDR_START);
    lcd_send_data(y);

    lcd_send_reg(LCD_REG_HORIZ_ADDR_END);
    lcd_send_data(y + height - 1);

    lcd_send_reg(LCD_REG_VERT_ADDR_START);
    lcd_send_data(x + x_offset);

    lcd_send_reg(LCD_REG_VERT_ADDR_END);
    lcd_send_data(x + width - 1 + x_offset);

    lcd_send_reg(LCD_REG_WRITE_DATA_2_GRAM);

    const int stride_div_csub_x = stride/CSUB_X;

    h=0;
    while (1)
    {
        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;

        const int uvoffset = stride_div_csub_x * (src_y/CSUB_Y) +
                             (src_x/CSUB_X);

        const unsigned char *usrc = src[1] + uvoffset;
        const unsigned char *vsrc = src[2] + uvoffset;

        int pixels_to_write;

        if (h==0)
        {
            while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_READY));
            LCD2_BLOCK_CONFIG = 0;

            if (height == 0) break;

            pixels_to_write = (width * height) * 2;
            h = height;

            /* calculate how much we can do in one go */
            if (pixels_to_write > 0x10000)
            {
                h = (0x10000/2) / width;
                pixels_to_write = (width * h) * 2;
            }

            height -= h;
            LCD2_BLOCK_CTRL = 0x10000080;
            LCD2_BLOCK_CONFIG = 0xc0010000 | (pixels_to_write - 1);
            LCD2_BLOCK_CTRL = 0x34000000;
        }

        lcd_yuv_write_inner_loop(ysrc,usrc,vsrc,width);

        src_y++;
        h--;
    }

    while (!(LCD2_BLOCK_CTRL & LCD2_BLOCK_READY));
    LCD2_BLOCK_CONFIG = 0;
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

    lcd_send_reg(LCD_REG_HORIZ_ADDR_START);
    lcd_send_data(y);

    lcd_send_reg(LCD_REG_HORIZ_ADDR_END);
    lcd_send_data(y + height - 1);

    lcd_send_reg(LCD_REG_VERT_ADDR_START);
    lcd_send_data(x + x_offset);

    lcd_send_reg(LCD_REG_VERT_ADDR_END);
    lcd_send_data(x + width - 1 + x_offset);

    lcd_send_reg(LCD_REG_WRITE_DATA_2_GRAM);

    addr = (unsigned long*)FBADDR(x,y);

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
