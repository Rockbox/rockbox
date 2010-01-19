/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Thomas Martitz
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
#include "backlight.h"
#include "lcd.h"

#ifdef HAVE_LCD_SLEEP
extern void lcd_awake(void);
#endif
/* in uisimulator/sdl/lcd-bitmap.c and lcd-charcell.c */
extern void sim_backlight(int value);

static inline int normalize_backlight(int val)
{
    /* normalize to xx% brightness for sdl */
    return ((val - MIN_BRIGHTNESS_SETTING + 1) * 100)/MAX_BRIGHTNESS_SETTING;
}

bool _backlight_init(void)
{
    return true;
}

void _backlight_on(void)
{
#if defined(HAVE_LCD_ENABLE)
    lcd_enable(true);
#elif defined(HAVE_LCD_SLEEP)
    lcd_awake();
#endif
#if (CONFIG_BACKLIGHT_FADING != BACKLIGHT_FADING_SW_SETTING)
    /* if we set the brightness to the settings value, then fading up
     * is glitchy */
    sim_backlight(normalize_backlight(backlight_brightness));
#endif
}

void _backlight_off(void)
{
    sim_backlight(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false);
#endif
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void _backlight_set_brightness(int val)
{
    sim_backlight(normalize_backlight(val));
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
#ifdef HAVE_BUTTON_LIGHT
void _buttonlight_on(void)
{
}

void _buttonlight_off(void)
{
}

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void _buttonlight_set_brightness(int val)
{
    (void)val;
}
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */
#endif /* HAVE_BUTTON_LIGHT */

#ifdef HAVE_REMOTE_LCD
void _remote_backlight_on(void)
{
    sim_remote_backlight(100);
}

void _remote_backlight_off(void)
{
    sim_remote_backlight(0);
}
#endif /* HAVE_REMOTE_LCD */
