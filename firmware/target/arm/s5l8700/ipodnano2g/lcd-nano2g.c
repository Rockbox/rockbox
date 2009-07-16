/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

/** hardware access functions */

static void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS&0x10);
    LCD_WCMD = cmd;
}

static void s5l_lcd_write_data(int data)
{
    LCD_WDATA = data;
    while (LCD_STATUS&0x10);
}


/** globals **/

static int xoffset; /* needed for flip */

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
}

void lcd_set_invert_display(bool yesno)
{
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    /* TODO: flip mode isn't working.  The commands in the else part of
       this function are how the original firmware inits the LCD */

    if (yesno)
    {
        xoffset = 132 - LCD_WIDTH; /* 132 colums minus the 128 we have */
    }
    else 
    {
        xoffset = 0;
    }
}



void lcd_off(void)
{
}

void lcd_on(void)
{
}

/* LCD init */
void lcd_init_device(void)
{
}


/*** Update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    /* Copy display bitmap to hardware */
    while (bheight--)
    {
    }
}


/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase_blit(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    (void)values;
    (void)phases;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
}


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int x,y;
    fb_data* p;
    fb_data pixel;

    s5l_lcd_write_cmd(0x3a);
    s5l_lcd_write_data(0x65);

    s5l_lcd_write_cmd(0x2a);
    s5l_lcd_write_data(0);
    s5l_lcd_write_data(0);
    s5l_lcd_write_data(0);
    s5l_lcd_write_data(LCD_WIDTH-1);

    s5l_lcd_write_cmd(0x2b);
    s5l_lcd_write_data(0);
    s5l_lcd_write_data(0);
    s5l_lcd_write_data(0);
    s5l_lcd_write_data(LCD_HEIGHT-1);

    s5l_lcd_write_cmd(0x2c);

    /* Copy display bitmap to hardware */

    p = &lcd_framebuffer[0][0];
    for (y = 0; y < LCD_HEIGHT; y++) {
        for (x = 0; x < LCD_WIDTH; x++) {
           pixel = *(p++);

           while (LCD_STATUS&0x10);
           LCD_WDATA = (pixel & 0xff00) >> 8;
           LCD_WDATA = pixel & 0xff;
        }
    }

    s5l_lcd_write_cmd(0x29);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;

    lcd_update();
}
