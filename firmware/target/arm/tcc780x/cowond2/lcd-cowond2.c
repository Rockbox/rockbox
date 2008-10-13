/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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

/* GPIO A pins for LCD panel SDI interface */

#define LTV250QV_CS  (1<<24)
#define LTV250QV_SCL (1<<25)
#define LTV250QV_SDI (1<<26)

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

/* Framebuffer copy as seen by the hardware */
fb_data lcd_driver_framebuffer[LCD_FBHEIGHT][LCD_FBWIDTH];

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    /* TODO: This won't be implemented until the S6F2002 controller
       is better understood (nb: registers 16-23 control gamma). */
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
    
    GPIOA_CLEAR = LTV250QV_CS;
    DELAY;

    for (i = 23; i >= 0; i--)
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
    GPIOA_SET = LTV250QV_CS;
}

static void lcd_write_reg(unsigned char reg, unsigned short val)
{
    ltv250qv_write(0x740000 | reg);
    ltv250qv_write(0x760000 | val);
}


/*
   TEMP: Rough millisecond delay routine used by the LCD panel init sequence.
   PCK_TCT must first have been initialised to 2Mhz by calling clock_init().
*/
static void sleep_ms(unsigned int ms)
{
    /* disable timer */
    TCFG1 = 0;

    /* set Timer1 reference value based on 125kHz tick */
    TREF1 = ms * 125;

    /* single count, zero the counter, divider = 16 [2^(3+1)], enable */
    TCFG1 = (1<<9) | (1<<8) | (3<<4) | 1;

    /* wait until Timer1 ref reached */
    while (!(TIREQ & TF1)) {};
}


static void lcd_display_on(void)
{
    /* power on sequence as per the D2 firmware */
    GPIOA_SET = (1<<16);
    
    sleep_ms(10);
    
    lcd_write_reg(1,  0x1D);
    lcd_write_reg(2,  0x0);
    lcd_write_reg(3,  0x0);
    lcd_write_reg(4,  0x0);
    lcd_write_reg(5,  0x40A3);
    lcd_write_reg(6,  0x0);
    lcd_write_reg(7,  0x0);
    lcd_write_reg(8,  0x0);
    lcd_write_reg(9,  0x0);
    lcd_write_reg(10, 0x0);
    lcd_write_reg(16, 0x0);
    lcd_write_reg(17, 0x0);
    lcd_write_reg(18, 0x0);
    lcd_write_reg(19, 0x0);
    lcd_write_reg(20, 0x0);
    lcd_write_reg(21, 0x0);
    lcd_write_reg(22, 0x0);
    lcd_write_reg(23, 0x0);
    lcd_write_reg(24, 0x0);
    lcd_write_reg(25, 0x0);
    sleep_ms(10);
    
    lcd_write_reg(9,  0x4055);
    lcd_write_reg(10, 0x0);
    sleep_ms(40);
    
    lcd_write_reg(10, 0x2000);
    sleep_ms(40);
    
    lcd_write_reg(1,  0xC01D);
    lcd_write_reg(2,  0x204);
    lcd_write_reg(3,  0xE100);
    lcd_write_reg(4,  0x1000);
    lcd_write_reg(5,  0x5033);
    lcd_write_reg(6,  0x2);     /* vertical back porch adjusted from 0x4 in OF */
    lcd_write_reg(7,  0x30);
    lcd_write_reg(8,  0x41C);
    lcd_write_reg(16, 0x207);
    lcd_write_reg(17, 0x702);
    lcd_write_reg(18, 0xB05);
    lcd_write_reg(19, 0xB05);
    lcd_write_reg(20, 0x707);
    lcd_write_reg(21, 0x507);
    lcd_write_reg(22, 0x103);
    lcd_write_reg(23, 0x406);
    lcd_write_reg(24, 0x2);
    lcd_write_reg(25, 0x0);
    sleep_ms(60);
    
    lcd_write_reg(9,  0xA55);
    lcd_write_reg(10, 0x111F);
    sleep_ms(10);

    /* tell that we're on now */
    display_on = true;
}

static void lcd_display_off(void)
{
    /* block drawing operations and changing of first */
    display_on = false;

    /* LQV shutdown sequence */
    lcd_write_reg(9,  0x55);
    lcd_write_reg(10, 0x1417);
    lcd_write_reg(5,  0x4003);
    sleep_ms(10);
    
    lcd_write_reg(9, 0x0);
    sleep_ms(10);
    
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
        lcd_call_enable_hook();
    }
    else
    {
        LCDC_CTRL &= ~1;    /* controller disable */
        lcd_display_off();
    }
}

bool lcd_enabled(void)
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
    PORTCFG1 &= ~0xC0000000;
    PORTCFG1 &= ~0x3FC0;
    PORTCFG2 &= ~0x100;
    
    /* set physical display size */
    LCDC_DS = (LCD_HEIGHT<<16) | LCD_WIDTH;

    LCDC_HTIME1 = (0x2d<<16) | 0x3bf;
    LCDC_HTIME2 = (1<<16) | 1;
    LCDC_VTIME1 = LCDC_VTIME3 = (0<<16) | 239;
    LCDC_VTIME2 = LCDC_VTIME4 = (1<<16) | 3;

    LCDC_I1BASE  = (unsigned int)lcd_driver_framebuffer;
    LCDC_I1SIZE  = (LCD_HEIGHT<<16) | LCD_WIDTH;  /* image 1 size */
    LCDC_I1POS   = (0<<16) | 0;                   /* position */
    LCDC_I1OFF   = 0;                             /* address offset */
    LCDC_I1SCALE = 0;                             /* scaling */
    LCDC_I1CTRL  = 5;                             /* 565bpp (7 = 888bpp) */
    LCDC_CTRL &= ~(1<<28);

    LCDC_CLKDIV = (LCDC_CLKDIV &~ 0xFF00FF) | (1<<16) | 2; /* and this means? */
    
    /* set and clear various flags - not investigated yet */
    LCDC_CTRL &~ 0x090006AA;  /* clear bits 1,3,5,7,9,10,24,27 */
    LCDC_CTRL |= 0x02800144;  /*   set bits 2,6,8,25,23 */
    LCDC_CTRL = (LCDC_CTRL &~ 0xF0000) | 0x20000;
    LCDC_CTRL = (LCDC_CTRL &~ 0x700000) | 0x700000;

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

    lcd_copy_buffer_rect(&lcd_driver_framebuffer[0][0],
                         &lcd_framebuffer[0][0], LCD_WIDTH*LCD_HEIGHT, 1);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

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

    /* TODO: It may be faster to swap the addresses of lcd_driver_framebuffer
     * and lcd_framebuffer */
    dst = &lcd_driver_framebuffer[y][x];
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
    off_t z;

    if (!display_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    fb_data *dst = &lcd_driver_framebuffer[y][x];

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither(dst, yuv_src, width, stride, y, x);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst += 2*LCD_FBWIDTH;
            y -= 2;
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
}
