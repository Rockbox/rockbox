/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2008 by Rob Purchase
 * Initial YP-P2 adaptation by soaa (Rockbox FS#10280).
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

/* Panel control pins: CS=C23, clock=A11, data=A10.
 * The RGB scanout bus carries framebuffer pixels. */

#define LTV250QV_CS  (1<<23)
#define LTV250QV_SCL (1<<11)
#define LTV250QV_SDI (1<<10)

/* LCD Controller registers */

#define LCDC_CTRL    (*(volatile unsigned long *)0xF0000000)
#define LCDC_CLKDIV  (*(volatile unsigned long *)0xF0000008)
#define LCDC_HTIME1  (*(volatile unsigned long *)0xF000000C)
#define LCDC_HTIME2  (*(volatile unsigned long *)0xF0000010)
#define LCDC_VTIME1  (*(volatile unsigned long *)0xF0000014)
#define LCDC_VTIME2  (*(volatile unsigned long *)0xF0000018)
#define LCDC_VTIME3  (*(volatile unsigned long *)0xF000001C)
#define LCDC_VTIME4  (*(volatile unsigned long *)0xF0000020)
#define LCDC_DS      (*(volatile unsigned long *)0xF000005C)
#define LCDC_I1CTRL  (*(volatile unsigned long *)0xF000008C)
#define LCDC_I1POS   (*(volatile unsigned long *)0xF0000090)
#define LCDC_I1SIZE  (*(volatile unsigned long *)0xF0000094)
#define LCDC_I1BASE  (*(volatile unsigned long *)0xF0000098)
#define LCDC_I1OFF   (*(volatile unsigned long *)0xF00000A8)
#define LCDC_I1SCALE (*(volatile unsigned long *)0xF00000AC)

/* Power and display status */
static bool display_on  = false; /* Is the display turned on? */

static unsigned lcd_yuv_options = 0;

/* Rockbox draws the usable portrait area inside the top and right margins.
 * The controller still scans all 480x272 native landscape pixels. */
#define LCD_NATIVE_WIDTH  480
#define LCD_NATIVE_HEIGHT 272

#if LCD_WIDTH + LCD_RIGHT_MARGIN != LCD_NATIVE_HEIGHT || LCD_HEIGHT + LCD_TOP_MARGIN != LCD_NATIVE_WIDTH
#error YP-P2 usable framebuffer and margins must fit the native panel
#endif

static fb_data lcd_driver_framebuffer[LCD_NATIVE_HEIGHT][LCD_NATIVE_WIDTH];

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    /* TODO: implement panel contrast and gamma control. */
    (void)val;
}


/* LTV250QV panel functions */

/* Delay loop based on CPU frequency (FREQ>>23 is 3..22 for 32MHz..192MHz) */
static void delay_loop(void)
{
    unsigned long x;
    for (x = (unsigned)(FREQ>>23); x; x--);
}
#define DELAY delay_loop()

static void ltv250qv_write(unsigned int command)
{
    int i;

    GPIOC_CLEAR = LTV250QV_CS;
    DELAY;

    for (i = 15; i >= 0; i--)
    {
      GPIOA_CLEAR = LTV250QV_SCL;
      DELAY;

      if ((command>>i) & 1)
        GPIOA_SET = LTV250QV_SDI;
      else
        GPIOA_CLEAR = LTV250QV_SDI;

      DELAY;
      GPIOA_SET = LTV250QV_SCL;
    }

    DELAY;
    GPIOC_SET = LTV250QV_CS;
}

static void lcd_write_reg(unsigned char reg, unsigned char val)
{
    int level = disable_irq_save();

    /* Each panel write uses one 16-bit frame: write bit (0), seven address
     * bits, then eight data bits. */
    ltv250qv_write(((unsigned int)reg << 8) | val);

    restore_irq(level);
}

static void lcd_display_on(void)
{
    /* Retain the existing P2 power-on values, using single-frame writes. */
    GPIOA_SET = (1<<16);
    udelay(10000);

    lcd_write_reg(0x0d, 0x44);
    lcd_write_reg(0x0d, 0x4d);
    lcd_write_reg(0x0b, 0x06);
    lcd_write_reg(0x40, 0xc0);
    lcd_write_reg(0x42, 0x43);
    lcd_write_reg(0x44, 0x28);
    lcd_write_reg(0x0d, 0x4f);

    /* tell that we're on now */
    display_on = true;
}

static void lcd_display_off(void)
{
    /* block drawing operations and changing of first */
    display_on = false;

    /* LQV shutdown sequence */
    lcd_write_reg(0x0d, 0x4c);

    /* kill power to LCD panel (unconfirmed) */
    GPIOA_CLEAR = (1<<16);
}


void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
    {
        lcd_display_on();
        LCDC_CTRL |= 1;     /* controller enable */
        lcd_update();       /* Resync display */
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        LCDC_CTRL &= ~1;    /* controller disable */
        lcd_display_off();
    }
}

bool lcd_active(void)
{
    return display_on;
}

/* TODO: implement lcd_sleep() and separate out the power on/off functions */

void lcd_init_device(void)
{
    BCLKCTR |= 4;   /* enable LCD bus clock */

    /* set PCK_LCD to 108Mhz */
    PCLK_LCD &= ~PCK_EN;
    PCLK_LCD = PCK_EN | (CKSEL_PLL1<<24) | 1;  /* source = PLL1, divided by 2 */

    /* reset the LCD controller */
    SWRESET |= 4;
    SWRESET &= ~4;

    /* set port configuration */
    /* Samsung's pin mux configuration enables the panel's RGB565 scanout. */
    PORTCFG1 |= 0xC0000000;
    PORTCFG1 &= ~0x3FC0;
    PORTCFG2 &= ~0x100;

    /* set physical display size */
    LCDC_DS = (LCD_NATIVE_HEIGHT<<16) | LCD_NATIVE_WIDTH;

    LCDC_HTIME1 = (0x29<<16) | 0x1df;
    LCDC_HTIME2 = (1<<16) | 1;
    LCDC_VTIME1 = LCDC_VTIME3 = (0<<16) | 271;
    LCDC_VTIME2 = LCDC_VTIME4 = (3<<16) | 5;

    /* Portrait top rows are native columns; the right edge becomes the
     * first native rows. Keep both unused strips black on every init. */
    for (int row = 0; row < LCD_NATIVE_HEIGHT; row++)
        for (int col = 0; col < LCD_NATIVE_WIDTH; col++)
            if (row < LCD_RIGHT_MARGIN || col < LCD_TOP_MARGIN)
                lcd_driver_framebuffer[row][col] = 0;

    LCDC_I1BASE  = (unsigned int)lcd_driver_framebuffer;
    LCDC_I1SIZE  = (LCD_NATIVE_HEIGHT<<16) | LCD_NATIVE_WIDTH;
    LCDC_I1POS   = (0<<16) | 0;                   /* position */
    LCDC_I1OFF   = 0;                             /* address offset */
    LCDC_I1SCALE = 0;                             /* scaling */
    LCDC_I1CTRL  = 5;                             /* 565bpp (7 = 888bpp) */
    LCDC_CTRL &= ~(1<<28);

    LCDC_CLKDIV = (LCDC_CLKDIV &~ 0xFF00FF) | (1<<16) | 7; /* and this means? */

    /* Panel timing and output polarity from the original initialization. */
    LCDC_CTRL &= ~0x090006AA; /* clear bits 1,3,5,7,9,10,24,27 */
    LCDC_CTRL |= 0x02806144;  /*   set bits 2,6,8,25,23 */
    LCDC_CTRL = (LCDC_CTRL &~ 0xF0000) | 0x50000;
    LCDC_CTRL = (LCDC_CTRL &~ 0xF00000) | 0x500000;

    /* enable LCD controller */
    LCDC_CTRL |= 1;

    /* enable LTV250QV panel */
    lcd_display_on();
}


/*** Update functions ***/


/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    if (!display_on)
        return;

    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    if (!display_on)
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

    /* Rotate 90 degrees counter-clockwise:
     *   native_x = logical_y + LCD_TOP_MARGIN
     *   native_y = LCD_NATIVE_HEIGHT - 1 - logical_x
     * Anchor the rotation to the full native panel width.
     * Each logical column becomes one contiguous native scanline. */
    for (int logical_x = x; logical_x < x + width; logical_x++)
    {
        fb_data *dst = &lcd_driver_framebuffer[LCD_NATIVE_HEIGHT - 1 - logical_x]
                                             [y + LCD_TOP_MARGIN];
        const fb_data *src = FBADDR(logical_x, y);

        for (int i = 0; i < height; i++)
        {
            *dst++ = *src;
            src += LCD_FBWIDTH;
        }
    }
}

void lcd_set_flip(bool yesno)
{
    // TODO
    (void)yesno;
}

void lcd_set_invert_display(bool yesno)
{
    // TODO
    (void)yesno;
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);

extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    long z;
    int output_height = height;
    int y_screen = y;

    if (!display_on)
        return;

    /* YUV420 processes pairs of rows and columns. */
    width &= ~1;
    height >>= 1;

    fb_data *dst = FBADDR(x, y);

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither(dst, yuv_src, width, stride,
                                           x, y_screen);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst += 2*LCD_FBWIDTH;
            y_screen += 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_write_yuv420_lines(dst, yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst += 2*LCD_FBWIDTH;
        }
        while (--height > 0);
    }

    lcd_update_rect(x, y, width, output_height);
}
