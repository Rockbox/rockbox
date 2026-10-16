/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "system.h"
#include "button-console.h"
#include "screendump.h"
#include "lcd-target.h"

#include "sys_display.h"
#include "sys_kernel_hook.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

/* True if we're in sleep mode */
static bool lcd_sleeping = false;
static bool lcd_on = false;

extern struct frame_buffer_t lcd_framebuffer_default;

void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *src;

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

    src = FBADDR(x,y);

    sys_blit_surface_rect(src, x, y, width, height);
    sys_flip_screen();
}

void lcd_update(void)
{
    /* update a full screen rect */
    sys_blit_surface(FBADDR(0,0));
    sys_flip_screen();
}

void lcd_init_device(void)
{
    sys_set_video_mode(LCD_DEPTH);
    lcd_on = true;
}

void lcd_shutdown(void)
{
    /* sys_delete_video_mode(); */
    lcd_on = false;
}

int lcd_get_dpi(void)
{
    /* all values for bottom lcd */
    float dpi = 128.3f;
    return (int) roundf(dpi);
}

#if defined(HAVE_LCD_ENABLE)
bool lcd_active(void)
{
    return lcd_on;
}

void lcd_enable(bool on)
{
    bool state = lcd_active();
    if(state && !on) {
        sys_display_disable();
        lcd_on = false;
    }

    /* Handle turning the LCD back on */
    if(!state && on)
    {
        sys_display_enable();
        lcd_on = true;
    }
}
#endif
