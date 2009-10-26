/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Dave Chapman
 *
 * LCD driver for the Sansa Fuze - controller unknown
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
#include "file.h"
#include "debug.h"
#include "system.h"
#include "clock-target.h"

/* The controller is unknown, but some registers appear to be the same as the
   HD66789R */

#define R_ENTRY_MODE            0x03
#define R_RAM_ADDR_SET          0x21
#define R_WRITE_DATA_2_GRAM     0x22

#define R_ENTRY_MODE_HORZ 0x7030
#define R_ENTRY_MODE_VERT 0x7038

static unsigned lcd_yuv_options = 0;
static bool display_on = false; /* is the display turned on? */
static bool display_flipped = false;
static int xoffset = 20; /* needed for flip */
/* we need to write a red pixel for correct button reads
 * (see lcd_button_support()),but that must not happen while the lcd is updating
 * so block lcd_button_support the during updates */
static bool lcd_busy = false;

static inline void lcd_delay(int x)
{
    do {
        asm volatile ("nop\n");
    } while (x--);
}
    
static void as3525_dbop_init(void)
{
    CGU_DBOP = (1<<3) | AS3525_DBOP_DIV;

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;

    /* short count: 16 | output data width: 16 | readstrobe line */
    DBOP_CTRL = (1<<18|1<<12|1<<3);

    GPIOB_AFSEL = 0xfc;
    GPIOC_AFSEL = 0xff;

    DBOP_TIMPOL_23 = 0x6000e;
    /* short count: 16|enable write|output data width: 16|read strobe line */
    DBOP_CTRL = (1<<18|1<<16|1<<12|1<<3);
    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;

    /* TODO: The OF calls some other functions here, but maybe not important */
}

static void lcd_write_value16(unsigned short value)
{
    DBOP_CTRL &= ~(1<<14|1<<13);
    lcd_delay(10);
    DBOP_DOUT16 = value;
    while ((DBOP_STAT & (1<<10)) == 0);
}

static void lcd_write_cmd(int cmd)
{
    /* Write register */
    DBOP_TIMPOL_23 = 0xa167006e;
    lcd_write_value16(cmd);

    /* Wait for fifo to empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    /* This loop is unique to the Fuze */
    lcd_delay(4);

    DBOP_TIMPOL_23 = 0xa167e06f;
}

void lcd_write_data(const fb_data* p_bytes, int count)
{
    const long *data;
    if ((int)p_bytes & 0x3)
    {   /* need to do a single 16bit write beforehand if the address is
         * not word aligned*/
        lcd_write_value16(*p_bytes);
        count--;p_bytes++;
    }
    /* from here, 32bit transfers are save */
    /* set it to transfer 4*(outputwidth) units at a time,
     * if bit 12 is set it only does 2 halfwords though */
    DBOP_CTRL |= (1<<13|1<<14);
    lcd_delay(10);
    data = (long*)p_bytes;
    while (count > 1)
    {
        DBOP_DOUT32 = *data++;
        count -= 2;

        /* Wait if push fifo is full */
        while ((DBOP_STAT & (1<<6)) != 0);
    }
    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    /* due to the 32bit alignment requirement or uneven count,
     * we possibly need to do a 16bit transfer at the end also */
    if (count > 0)
        lcd_write_value16(*(unsigned short*)data);
}

static void lcd_write_reg(int reg, int value)
{
    unsigned short data = value;

    lcd_write_cmd(reg);
    lcd_write_value16(data);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    display_flipped = yesno;
    xoffset = yesno ? 0 : 20;   /* TODO: Implement flipped mode */

    /* TODO */
}


static void _display_on(void)
{
    /* Initialise in the same way as the original firmare */

    lcd_write_reg(0x07, 0);
    lcd_write_reg(0x13, 0);

    lcd_write_reg(0x11, 0x3704);
    lcd_write_reg(0x14, 0x1a1b);
    lcd_write_reg(0x10, 0x3860);
    lcd_write_reg(0x13, 0x40);

    lcd_write_reg(0x13, 0x60);

    lcd_write_reg(0x13, 0x70);
    lcd_write_reg(0x01, 277);
    lcd_write_reg(0x02, (7<<8));
    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);
    lcd_write_reg(0x08, 0x01);
    lcd_write_reg(0x0b, (1<<10));
    lcd_write_reg(0x0c, 0);

    lcd_write_reg(0x30, 0x40);
    lcd_write_reg(0x31, 0x0687);
    lcd_write_reg(0x32, 0x0306);
    lcd_write_reg(0x33, 0x104);
    lcd_write_reg(0x34, 0x0585);
    lcd_write_reg(0x35, 255+66);
    lcd_write_reg(0x36, 0x0687+128);
    lcd_write_reg(0x37, 259);
    lcd_write_reg(0x38, 0);
    lcd_write_reg(0x39, 0);

    lcd_write_reg(0x42, (LCD_WIDTH - 1));
    lcd_write_reg(0x43, 0);
    lcd_write_reg(0x44, (LCD_WIDTH - 1));
    lcd_write_reg(0x45, 0);
    lcd_write_reg(0x46, (((LCD_WIDTH - 1) + xoffset) << 8) | xoffset);
    lcd_write_reg(0x47, (LCD_HEIGHT - 1));
    lcd_write_reg(0x48, 0x0);

    lcd_write_reg(0x07, 0x11);
    lcd_write_reg(0x07, 0x17);

    display_on = true;  /* must be done before calling lcd_update() */
    lcd_update();
}

#if defined(HAVE_LCD_ENABLE)
void lcd_enable(bool on)
{
    if (display_on == on)
        return; /* nothing to do */
    if(on)
    {
        lcd_write_reg(0, 1);
        lcd_write_reg(0x10, 0);
        lcd_write_reg(0x11, 0x3704);
        lcd_write_reg(0x14, 0x1a1b);
        lcd_write_reg(0x10, 0x3860);
        lcd_write_reg(0x13, 0x40);
        lcd_write_reg(0x13, 0x60);
        lcd_write_reg(0x13, 112);
        lcd_write_reg(0x07, 0x11);
        lcd_write_reg(0x07, 0x17);
        display_on = true;
        lcd_update();      /* Resync display */
        send_event(LCD_EVENT_ACTIVATION, NULL);
        sleep(0);

    }
    else
    {
        lcd_write_reg(0x07, 0x22);
        lcd_write_reg(0x07, 0);
        lcd_write_reg(0x10, 1);
        display_on = false;
    }
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return display_on;
}
#endif

/*** update functions ***/

/* Set horizontal window addresses */
static void lcd_window_x(int xmin, int xmax)
{
    xmin += xoffset;
    xmax += xoffset;
    lcd_write_reg(0x46, (xmax << 8) | xmin);
    lcd_write_reg(0x20, xmin);
}

/* Set vertical window addresses */
static void lcd_window_y(int ymin, int ymax)
{
    lcd_write_reg(0x47, ymax);
    lcd_write_reg(0x48, ymin);
    lcd_write_reg(0x21, ymin);
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither */
                                           int y_screen); /*  pattern */
/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    lcd_busy = true;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_VERT);

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    lcd_window_x(x, x + width - 1);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_window_y(y, y + 1);
            /* Start write to GRAM */
            lcd_write_cmd(R_WRITE_DATA_2_GRAM);

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
            lcd_window_y(y, y + 1);
            /* Start write to GRAM */
            lcd_write_cmd(R_WRITE_DATA_2_GRAM);

            lcd_write_yuv420_lines(yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
         while (--height > 0);
    }

    lcd_busy = false;
}

void lcd_init_device()
{
    as3525_dbop_init();

    GPIOA_DIR |= (1<<5|1<<4|1<<3);
    GPIOA_PIN(5) = 0;
    GPIOA_PIN(3) = (1<<3);
    GPIOA_PIN(4) = 0;
    GPIOA_PIN(5) = (1<<5);

    _display_on();
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!display_on)
        return;

    lcd_busy = true;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);

    lcd_window_x(0, LCD_WIDTH - 1);
    lcd_window_y(0, LCD_HEIGHT - 1);

    /* Start write to GRAM */
    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    /* Write data */
    lcd_write_data((unsigned short *)lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT);
    lcd_busy = false;
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int xmax, ymax;
    const fb_data *ptr;

    if (!display_on)
        return;


    xmax = x + width;
    if (xmax >= LCD_WIDTH)
        xmax = LCD_WIDTH - 1; /* Clip right */
    if (x < 0)
        x = 0;                /* Clip left */
    if (x >= xmax)
        return; /* nothing left to do */

    width = xmax - x + 1;     /* Fix width */

    ymax = y + height;
    if (ymax >= LCD_HEIGHT)
        ymax = LCD_HEIGHT - 1; /* Clip bottom */
    if (y < 0)
        y = 0; /* Clip top */
    if (y >= ymax)
        return; /* nothing left to do */

    lcd_busy = true;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);

    lcd_window_x(x, xmax);
    lcd_window_y(y, ymax);

    /* Start write to GRAM */
    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    ptr = &lcd_framebuffer[y][x];

    height = ymax - y - 1; /* fix height */
    do
    {
        lcd_write_data(ptr, width);
        ptr += LCD_WIDTH;
    }
    while (--height >= 0);
    lcd_busy = false;
}

/* writes one read pixel outside the visible area, needed for correct dbop reads */
bool lcd_button_support(void)
{
    fb_data data = 0xf<<12;
    if (lcd_busy)
        return false;
    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_HORZ);
    /* Set start position and window */

    lcd_window_x(-1, 0);
    lcd_window_y(-1, 0);
    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    lcd_write_value16(data);

    return true;
}
