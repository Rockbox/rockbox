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

/* The LCD registers appear to match controllers from Leadis Technology,
   either the LDS176 (132x132 4k) or the LDS186 (128x160 65k).
   These defines are from the LDS176 (I couldn't find the LDS186 datasheet. */
#define NOP     0x00
#define SWRESET 0x01
#define BSTROFF 0x02
#define BSTRON  0x03
#define RDDID   0x04
#define RDDST   0x09
#define SLPIN   0x10
#define SLPOUT  0x11
#define PTLON   0x12
#define NORON   0x13
#define INVOFF  0x20
#define INVON   0x21
#define APOFF   0x22
#define APON    0x23
#define WRCNTR  0x25
#define DISPOFF 0x28
#define DISPON  0x29
#define CASET   0x2a
#define RASET   0x2b
#define RAMWR   0x2c
#define RAMRD   0x2e
#define RGBSET  0x2d
#define PTLAR   0x30
#define SCRLAR  0x33
#define TEOFF   0x34
#define TEON    0x35
#define MADCTR  0x36
#define VSCSAD  0x37
#define IDMOFF  0x38
#define IDMON   0x39
#define COLMOD  0x3a
#define RDID1   0xda
#define RDID2   0xdb
#define RDID3   0xdc
#define CLKINT  0xb0
#define CLKEXT  0xb1
#define FRMSEL  0xb4
#define FRM8SEL 0xb5
#define TMPRNG  0xb6
#define TMPHIS  0xb7
#define TMPREAD 0xb8
#define DISCTR  0xba
#define EPVOL   0xbb
#define EPWRIN  0xd1
#define EPWROUT 0xd0
#define RDEV    0xd4
#define RDRR    0xd5

/* Display status */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;
static unsigned mad_ctrl = 0;

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

    /* lcd power */
    GPIOJ_ENABLE     |= 0x4;
    GPIOJ_OUTPUT_VAL |= 0x4;
    GPIOJ_OUTPUT_EN  |= 0x4;
    
    lcd_send_cmd(SWRESET);
    udelay(10000);
    
    lcd_send_cmd(WRCNTR);
    lcd_send_data(0x3f);
    
    lcd_send_cmd(SLPOUT);
    udelay(120000);
    
    lcd_send_cmd(INVOFF);
    lcd_send_cmd(IDMOFF);
    lcd_send_cmd(NORON);
    
    lcd_send_cmd(FRMSEL);
    lcd_send_data(0x2);
    lcd_send_data(0x6);
    lcd_send_data(0x8);
    lcd_send_data(0xd);

    lcd_send_cmd(FRM8SEL);
    lcd_send_data(0x2);
    lcd_send_data(0x6);
    lcd_send_data(0x8);
    lcd_send_data(0xd);

    lcd_send_cmd(TMPRNG);
    lcd_send_data(0x19);
    lcd_send_data(0x23);
    lcd_send_data(0x2d);

    lcd_send_cmd(TMPHIS);
    lcd_send_data(0x5);

    lcd_send_cmd(DISCTR);
    lcd_send_data(0x7);
    lcd_send_data(0x18);

    lcd_send_cmd(MADCTR);
    lcd_send_data(0);
    mad_ctrl = 0;

    lcd_send_cmd(COLMOD);
    lcd_send_data(0x5);
    
    lcd_send_cmd(RGBSET);
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

    lcd_send_cmd(DISPON);
#endif
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_send_cmd(WRCNTR);
    lcd_send_data(val);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno) 
        lcd_send_cmd(INVON);
    else
        lcd_send_cmd(INVOFF);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno)
        mad_ctrl |= ((1<<7) | (1<<6));  /* flip */
    else
        mad_ctrl &= ~((1<<7) | (1<<6)); /* normal */
    
    lcd_send_cmd(MADCTR);
    lcd_send_data(mad_ctrl);
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width, int stride);

extern void lcd_write_yuv420_lines_odither(unsigned char const * const src[3],
                                           int width, int stride,
                                           int x_screen,  int y_screen);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;
    
    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    /* Set vertical address mode */
    lcd_send_cmd(MADCTR);
    lcd_send_data(mad_ctrl | (1<<5));

    lcd_send_cmd(RASET);
    lcd_send_data(x);
    lcd_send_data(x + width - 1);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_send_cmd(CASET);
            lcd_send_data(y);
            lcd_send_data(y + 1);

            lcd_send_cmd(RAMWR);

            lcd_write_yuv420_lines_odither(yuv_src, width, stride, x, y);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_send_cmd(CASET);
            lcd_send_data(y);
            lcd_send_data(y + 1);

            lcd_send_cmd(RAMWR);

            lcd_write_yuv420_lines(yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
        while (--height > 0);
    }

    /* Restore the address mode */
    lcd_send_cmd(MADCTR);
    lcd_send_data(mad_ctrl);
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

    lcd_send_cmd(CASET);
    lcd_send_data(x);
    lcd_send_data(x + width - 1);

    lcd_send_cmd(RASET);
    lcd_send_data(y);
    lcd_send_data(y + height - 1);

    lcd_send_cmd(RAMWR);
    do {
        int w = width;
        do {
            lcd_send_pixel(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
