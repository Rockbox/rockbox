/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2008 François Dinel
 * Copyright (C) 2008-2009 Rafaël Carré
 * Copyright (C) 2017 William Wilgus
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

#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "string.h"

#include "lcd-clip.h"

/*** definitions ***/

#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_SEGMENT_REMAP_INV                 ((char)0xA1)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_MULTIPLEX_RATIO                   ((char)0xA8)
#define LCD_SET_DC_DC                             ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV     ((char)0xC8)
#define LCD_SET_DISPLAY_CLOCK_AND_OSC_FREQ        ((char)0xD5)
#define LCD_SET_VCOM_DESELECT_LEVEL               ((char)0xDB)
#define LCD_SET_PRECHARGE_PERIOD                  ((char)0xD9)
#define LCD_NOP                                   ((char)0xE3)

/* LCD command codes */
#define LCD_CNTL_CONTRAST       0x81    /* Contrast */
#define LCD_CNTL_OUTSCAN        0xc8    /* Output scan direction */
#define LCD_CNTL_SEGREMAP       0xa1    /* Segment remap */
#define LCD_CNTL_DISPON         0xaf    /* Display on */

#define LCD_CNTL_PAGE           0xb0    /* Page address */
#define LCD_CNTL_HIGHCOL        0x10    /* Upper column address */
#define LCD_CNTL_LOWCOL         0x00    /* Lower column address */

/** globals **/

static bool display_on; /* used by lcd_enable */

/* Display variant, always 0 in clipv1, clipv2, can be 0 or 1 in clip+
 * variant 0: has 132 pixel wide framebuffer, max brightness about 50
 * variant 1: has 128 pixel wide framebuffer, max brightness about 128
 */
static int variant;

static int offset; /* column offset */

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_write_command(LCD_CNTL_CONTRAST);
    if (variant == 1) {
        val = val * 5 / 2;
    }
    lcd_write_command(val);
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno)
        lcd_write_command(LCD_SET_REVERSE_DISPLAY);
    else
        lcd_write_command(LCD_SET_NORMAL_DISPLAY);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno)
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION);
    }
    else
    {
        lcd_write_command(LCD_SET_SEGMENT_REMAP_INV);
        lcd_write_command(LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV);
    }
}

#ifdef HAVE_LCD_ENABLE
void lcd_enable(bool enable)
{
    if (display_on == enable)
        return;

    if ( (display_on = enable) ) /* simple '=' is not a typo ! */
    {
        lcd_enable_power(enable);
        lcd_write_command(LCD_SET_DISPLAY_ON);
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_write_command(LCD_SET_DISPLAY_OFF);
        lcd_enable_power(enable);
    }
}

bool lcd_active(void)
{
    return display_on;
}

#endif

/* LCD init, largely based on what OF does */
void lcd_init_device(void)
{
    int i;

    variant = lcd_hw_init();
    offset = (variant == 0) ? 2 : 0;

    /* power on display to accept commands */
    lcd_enable_power(true);

    lcd_write_cmd_triplet
    (
        /* Set display clock */
        (LCD_SET_DISPLAY_CLOCK_AND_OSC_FREQ),
        /* Set display clock (divide ratio = 1) and oscillator frequency (1) */
        (0x10),
        /* Set VCOM deselect level */
        (LCD_SET_VCOM_DESELECT_LEVEL)
    );

    lcd_write_cmd_triplet
    (
        /* Set VCOM deselect level to 0.76V */
        (0x34),
        /* Set pre-charge period */
        (LCD_SET_PRECHARGE_PERIOD),
        /* Set pre-charge period (p1period is 2 dclk and p2period is 5 dclk) */
        (0x25)
    );

    /* Set contrast register to 12% */
    lcd_set_contrast(lcd_default_contrast());

    lcd_write_cmd_triplet
    (
        /* Configure DC-DC */
        (LCD_SET_DC_DC),
        /* Configure DC-DC */
        ((variant == 0) ? 0x8A : 0x10),
        /* Set starting line as 0 */
        (LCD_SET_DISPLAY_START_LINE /*|(0 & 0x3f)*/)
    );

    lcd_write_cmd_triplet
    (
        /* Column 131 is remapped to SEG0 */
        (LCD_SET_SEGMENT_REMAP_INV),
        /* Invert COM scan direction (N-1 to 0) */
        (LCD_SET_COM_OUTPUT_SCAN_DIRECTION_INV),
        /* Set normal display mode (not every pixel ON) */
        (LCD_SET_ENTIRE_DISPLAY_OFF)
    );

    lcd_write_cmd_triplet
    (
        /* Set normal display mode (not inverted) */
        (LCD_SET_NORMAL_DISPLAY),
        /* set upper 4 bits of 8-bit column address */
        (LCD_SET_HIGHER_COLUMN_ADDRESS /*| 0*/),
        /* set lower 4 bits of 8-bit column address */
        (LCD_SET_LOWER_COLUMN_ADDRESS /*| 0*/)
    );

    fb_data p_bytes[LCD_WIDTH + 2 * offset];
    memset(p_bytes, 0, sizeof(p_bytes)); /* fills with 0 : pixel off */
    for (i = 0; i < 8; i++)
    {
        lcd_write_command (LCD_SET_PAGE_ADDRESS | (i /*& 0xf*/));
        lcd_write_data(p_bytes, LCD_WIDTH + 2 * offset);
    }

    lcd_enable(true);

    lcd_update();
}

/*** Update functions ***/

/* returns LCD_CNTL_HIGHCOL or'd with higher 4 bits of
   the 8-bit column address for the display data RAM.
*/
static inline int get_column_high_byte(const int x)
{
    return (LCD_CNTL_HIGHCOL | (((x+offset) >> 4) & 0xf));
}

/* returns LCD_CNTL_LOWCOL or'd with lower 4 bits of
   the 8-bit column address for the display data RAM.
*/
static inline int get_column_low_byte(const int x)
{
     return (LCD_CNTL_LOWCOL | ((x+offset) & 0xf));
}

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    if (!display_on)
        return;

    const int column_high = get_column_high_byte(x);
    const int column_low = get_column_low_byte(x);

    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (by++ & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_write_data(data, width);
        data += stride;
    }
}

#ifndef BOOTLOADER
/* Helper function for lcd_grey_phase_blit(). */
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count);

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    if (!display_on)
        return;

    const int column_high = get_column_high_byte(x);
    const int column_low = get_column_low_byte(x);

    stride <<= 3; /* 8 pixels per block */
    /* Copy display bitmap to hardware */
    while (bheight--)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (by++ & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_grey_data(values, phases, width);

        values += stride;
        phases += stride;
    }
}

#endif

/* Shared internal function for lcd_update and lcd_update_rect
   WARNING does NOT check bounds
*/
static void internal_update_rect(int, int, int, int) ICODE_ATTR;
static void internal_update_rect(int x, int y, int width, int height)
{
    if (!display_on)
        return;

    const int column_high = get_column_high_byte(x);
    const int column_low = get_column_low_byte(x);

    void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
    /* Copy specified rectange bitmap to hardware */
    for (; y <= height; y++)
    {
        lcd_write_cmd_triplet
        (
            (LCD_CNTL_PAGE | (y & 0xf)),
            (column_high),
            (column_low)
        );

        lcd_write_data (fbaddr(x,y), width);
    }
    lcd_write_command (LCD_NOP); /* return to command mode */

}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    /* Copy display bitmap to hardware */
    internal_update_rect(0, 0, LCD_WIDTH, LCD_FBHEIGHT - 1);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax;
    if (!display_on)
        return;

    /* make sure the rectangle is bounded in the screen */
    if (width > LCD_WIDTH - x)/* Clip right */
        width = LCD_WIDTH - x;
    if (x < 0)/* Clip left */
    {
        width += x;
        x = 0;
    }
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */

    if (height > LCD_HEIGHT - y) /* Clip bottom */
        height = LCD_HEIGHT - y;
    if (y < 0) /* Clip top */
    {
        height += y;
        y = 0;
    }
    if (height <= 0)
        return; /* nothing left to do */

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 3;
    y >>= 3;

    /* Copy specified rectange bitmap to hardware */
    internal_update_rect(x, y, width, ymax);
}
