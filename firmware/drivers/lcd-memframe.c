/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2007, 2011 Michael Sevakis
 *
 * Shared C code for memory framebuffer LCDs
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
#include "system.h"
#include "lcd.h"
#include "lcd-target.h"

/*** Misc. functions ***/
bool lcd_on SHAREDBSS_ATTR = false;

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return lcd_on;
}

/* For use by target driver only! */
void lcd_set_active(bool active)
{
    lcd_on = active;
}

#else
#define lcd_on true
#endif

#ifndef lcd_write_enabled
#define lcd_write_enabled() lcd_on
#endif


/*** Blitting functions ***/

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

#ifndef LCD_OPTIMIZED_UPDATE
/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_write_enabled())
        return;

    /* Copy the Rockbox framebuffer to the second framebuffer */
    lcd_copy_buffer_rect(LCD_FRAMEBUF_ADDR(0, 0), FBADDR(0,0),
                         LCD_WIDTH*LCD_HEIGHT, 1);
}
#endif /* LCD_OPTIMIZED_UPDATE */

#ifndef LCD_OPTIMIZED_UPDATE_RECT
/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (!lcd_write_enabled())
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

    dst = LCD_FRAMEBUF_ADDR(x, y);
    src = FBADDR(x,y);

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
#endif /* LCD_OPTIMIZED_UPDATE_RECT */
