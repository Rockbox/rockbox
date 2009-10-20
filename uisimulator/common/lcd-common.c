/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert E. Hak <rhak@ramapo.edu>
 *
 * Windows Copyright (C) 2002 by Felix Arends
 * X11 Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
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

#include <stdbool.h>
#include "config.h"
#include "system.h"
#include "lcd.h"

#ifdef HAVE_LCD_ENABLE
static bool lcd_enabled = false;
#endif
#ifdef HAVE_LCD_SLEEP
static bool lcd_sleeping = true;
#endif

void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

void lcd_set_invert_display(bool invert)
{
    (void)invert;
}

int lcd_default_contrast(void)
{
    return 28;
}

#ifdef HAVE_REMOTE_LCD
void lcd_remote_set_contrast(int val)
{
    (void)val;
}
void lcd_remote_backlight_on(int val)
{
    (void)val;
}
void lcd_remote_backlight_off(int val)
{
    (void)val;
}

void lcd_remote_set_flip(bool yesno)
{
    (void)yesno;
}

void lcd_remote_set_invert_display(bool invert)
{
    (void)invert;
}
#endif

#ifdef HAVE_LCD_SLEEP
void lcd_sleep(void)
{
    lcd_sleeping = true;
}

void lcd_awake(void)
{
    if (lcd_sleeping)
    {
        send_event(LCD_EVENT_ACTIVATION, NULL);
        lcd_sleeping = false;
    }
}
#endif
#ifdef HAVE_LCD_ENABLE
void lcd_enable(bool on)
{
    if (on && !lcd_enabled)
    {
#ifdef HAVE_LCD_SLEEP
        /* lcd_awake will handle the activation call */
        lcd_awake();
#else
        send_event(LCD_EVENT_ACTIVATION, NULL);
#endif
    }
    lcd_enabled = on;
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    bool retval = false;
#ifdef HAVE_LCD_ENABLE
    retval = lcd_enabled;
#endif
#ifdef HAVE_LCD_SLEEP
    if (!retval)
        retval = !lcd_sleeping;
#endif
    return retval;
}
#endif
