/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * Some of this is based on the Cowon A2 Firmware release:
 * http://www.cowonglobal.com/download/gnu/cowon_pmp_a2_src_1.59_GPL.tar.gz
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
#include "string.h"
#include "lcd.h"
#include "kernel.h"
#include "memory.h"
#include "system-target.h"
#include "lcd-target.h"

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

static volatile bool lcd_on = true;
volatile bool lcd_poweroff = false;
/*
** These are imported from lcd-16bit.c
*/
extern unsigned fg_pattern;
extern unsigned bg_pattern;

bool lcd_enabled(void)
{
    return lcd_on;
}

/* LCD init - based on code from ingenient-bsp/bootloader/board/dm320/splash.c
 *  and code by Catalin Patulea from the M:Robe 500i linux port
 */
void lcd_init_device(void)
{
    unsigned int addr;
    
    /* Clear the Frame */
    memset16(FRAME, 0x0000, LCD_WIDTH*LCD_HEIGHT);

    IO_OSD_MODE=0x00ff;
    IO_OSD_VIDWINMD=0x0002;
    IO_OSD_OSDWINMD0=0x2001;
    IO_OSD_OSDWINMD1=0x0002;
    IO_OSD_ATRMD=0x0000;
    IO_OSD_RECTCUR=0x0000;

    IO_OSD_OSDWIN0OFST=(480*2) / 32;
    addr = ((int)FRAME-CONFIG_SDRAM_START) / 32;
    IO_OSD_OSDWINADH=addr >> 16;
    IO_OSD_OSDWIN0ADL=addr & 0xFFFF;

    IO_OSD_BASEPX=80;
    IO_OSD_BASEPY=2;

    IO_OSD_OSDWIN0XP=0;
    IO_OSD_OSDWIN0YP=0;
    IO_OSD_OSDWIN0XL=480;
    IO_OSD_OSDWIN0YL=640;
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    register fb_data *dst, *src;

    if (!lcd_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;
    src = &lcd_framebuffer[y][x];

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
#else
    src = &lcd_framebuffer[y][x];
    
    register int xc, yc;
    register fb_data *start=FRAME + LCD_HEIGHT*(LCD_WIDTH-x-1) + y + 1;

    for(yc=0;yc<height;yc++)
    {
        dst=start+yc;
        for(xc=0; xc<width; xc++)
        {
            *dst=*src++;
            dst-=LCD_HEIGHT;
        }
        src+=x;
    }
#endif
}

void lcd_enable(bool state)
{
    (void)state;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_on)
        return;
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
                         LCD_WIDTH*LCD_HEIGHT, 1);
#else
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char chroma_buf[LCD_HEIGHT/2*3],
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recaculated every other
       line */
    unsigned char chroma_buf[LCD_HEIGHT/2*3]; /* 480 bytes */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!lcd_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + (LCD_WIDTH - y) - 1;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    do
    {
        lcd_write_yuv420_lines(dst, chroma_buf, yuv_src, width,
                               stride);

        yuv_src[0] += stride << 1; /* Skip down two luma lines */
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
        dst -= 2;
    }
    while (--height > 0);
}

void lcd_set_contrast(int val) {
  (void) val;
  // TODO:
}

void lcd_set_invert_display(bool yesno) {
  (void) yesno;
  // TODO:
}

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}

