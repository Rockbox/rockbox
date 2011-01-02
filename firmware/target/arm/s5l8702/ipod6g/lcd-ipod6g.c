/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-nano2g.c 28868 2010-12-21 06:59:17Z Buschel $
 *
 * Copyright (C) 2009 by Dave Chapman
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

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "pmu-target.h"
#include "power.h"


#define R_HORIZ_GRAM_ADDR_SET     0x200
#define R_VERT_GRAM_ADDR_SET      0x201
#define R_WRITE_DATA_TO_GRAM      0x202
#define R_HORIZ_ADDR_START_POS    0x210
#define R_HORIZ_ADDR_END_POS      0x211
#define R_VERT_ADDR_START_POS     0x212
#define R_VERT_ADDR_END_POS       0x213


/* LCD type 1 register defines */

#define R_COLUMN_ADDR_SET         0x2a
#define R_ROW_ADDR_SET            0x2b
#define R_MEMORY_WRITE            0x2c


/** globals **/

int lcd_type; /* also needed in debug-s5l8702.c */


static inline void s5l_lcd_write_cmd_data(int cmd, int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;

    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

static inline void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static inline void s5l_lcd_write_data(unsigned short data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

bool lcd_active(void)
{
    return true;
}

void lcd_shutdown(void)
{
    pmu_write(0x2b, 0);  /* Kill the backlight, instantly. */
    pmu_write(0x29, 0);

    if (lcd_type == 3)
    {
        s5l_lcd_write_cmd_data(0x7, 0x172);
        s5l_lcd_write_cmd_data(0x30, 0x3ff);
        sleep(HZ / 10);
        s5l_lcd_write_cmd_data(0x7, 0x120);
        s5l_lcd_write_cmd_data(0x30, 0x0);
        s5l_lcd_write_cmd_data(0x100, 0x780);
        s5l_lcd_write_cmd_data(0x7, 0x0);
        s5l_lcd_write_cmd_data(0x101, 0x260);
        s5l_lcd_write_cmd_data(0x102, 0xa9);
        sleep(HZ / 30);
        s5l_lcd_write_cmd_data(0x100, 0x700);
        s5l_lcd_write_cmd_data(0x100, 0x704);
    }
    else if (lcd_type == 1)
    {
        s5l_lcd_write_cmd(0x28);
        s5l_lcd_write_cmd(0x10);
        sleep(HZ / 10);
    }
    else
    {
        s5l_lcd_write_cmd(0x28);
        sleep(HZ / 20);
        s5l_lcd_write_cmd(0x10);
        sleep(HZ / 20);
    }
}

void lcd_sleep(void)
{
    lcd_shutdown();
}

/* LCD init */
void lcd_init_device(void)
{
    /* Detect lcd type */
    lcd_type = (PDAT6 & 0x30) >> 4;
}

/*** Update functions ***/

static inline void lcd_write_pixel(fb_data pixel)
{
    LCD_WDATA = pixel;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Line write helper function. */
extern void lcd_write_line(const fb_data *addr, 
                           int pixelcount,
                           const unsigned int lcd_base_addr);

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    fb_data* p;
    
    /* Both x and width need to be preprocessed due to asm optimizations */
    x     = x & ~1;                 /* ensure x is even */
    width = (width + 3) & ~3;       /* ensure width is a multiple of 4 */

    x0 = x;                         /* start horiz */
    y0 = y;                         /* start vert */
    x1 = (x + width) - 1;           /* max horiz */
    y1 = (y + height) - 1;          /* max vert */

    if (lcd_type & 2) {
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_START_POS, x0);
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_END_POS,   x1);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_START_POS,  y0);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_END_POS,    y1);

        s5l_lcd_write_cmd_data(R_HORIZ_GRAM_ADDR_SET,  (x1 << 8) | x0);
        s5l_lcd_write_cmd_data(R_VERT_GRAM_ADDR_SET,   (y1 << 8) | y0);

        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    } else {
        s5l_lcd_write_cmd(R_COLUMN_ADDR_SET);
        s5l_lcd_write_data(x0 >> 8);
        s5l_lcd_write_data(x0 & 0xff);
        s5l_lcd_write_data(x1 >> 8);
        s5l_lcd_write_data(x1 & 0xff);

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_data(y0 >> 8);
        s5l_lcd_write_data(y0 & 0xff);
        s5l_lcd_write_data(y1 >> 8);
        s5l_lcd_write_data(y1 & 0xff);

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            s5l_lcd_write_data(lcd_framebuffer[y][x]);
    return;

    /* Copy display bitmap to hardware */
    p = &lcd_framebuffer[y0][x0];
    if (LCD_WIDTH == width) {
        /* Write all lines at once */
        lcd_write_line(p, height*LCD_WIDTH, LCD_BASE);
    } else {
        y1 = height;
        do {
            /* Write a single line */
            lcd_write_line(p, width, LCD_BASE);
            p += LCD_WIDTH;
        } while (--y1 > 0 );
    }
}

/* Line write helper function for lcd_yuv_blit. Writes two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   const unsigned int lcd_baseadress,
                                   int width,
                                   int stride);

/* Blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned int z, y0, x0, y1, x1;;
    unsigned char const * yuv_src[3];
    
    width = (width + 1) & ~1;       /* ensure width is even */

    x0 = x;                         /* start horiz */
    y0 = y;                         /* start vert */
    x1 = (x + width) - 1;           /* max horiz */
    y1 = (y + height) - 1;          /* max vert */

    if (lcd_type & 2) {
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_START_POS, x0);
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_END_POS,   x1);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_START_POS,  y0);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_END_POS,    y1);

        s5l_lcd_write_cmd_data(R_HORIZ_GRAM_ADDR_SET,  (x1 << 8) | x0);
        s5l_lcd_write_cmd_data(R_VERT_GRAM_ADDR_SET,   (y1 << 8) | y0);

        s5l_lcd_write_cmd(0);
        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    } else {
        s5l_lcd_write_cmd(R_COLUMN_ADDR_SET);
        s5l_lcd_write_data(x0);            /* Start column */
        s5l_lcd_write_data(x1);            /* End column */

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_data(y0);            /* Start row */
        s5l_lcd_write_data(y1);            /* End row */

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    height >>= 1;

    do {
        lcd_write_yuv420_lines(yuv_src, LCD_BASE, width, stride);
        yuv_src[0] += stride << 1;
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
    } while (--height > 0);
}
