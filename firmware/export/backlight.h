/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include "config.h"



#if !defined(BOOTLOADER)
/* The whole driver should be built */
#define BACKLIGHT_FULL_INIT
#endif

bool is_backlight_on(bool ignore_always_off);
void backlight_on(void);
void backlight_off(void);
void backlight_set_timeout(int value);

#ifdef HAVE_BACKLIGHT
void backlight_init(void) INIT_ATTR;
void backlight_close(void);

int  backlight_get_current_timeout(void);

#if defined(HAVE_BACKLIGHT_FADING_INT_SETTING)
void backlight_set_fade_in(int value);
void backlight_set_fade_out(int value);
#elif defined(HAVE_BACKLIGHT_FADING_BOOL_SETTING)
void backlight_set_fade_in(bool value);
void backlight_set_fade_out(bool value);
#endif

void backlight_set_timeout_plugged(int value);

#ifdef HAS_BUTTON_HOLD
void backlight_hold_changed(bool hold_button);
void backlight_set_on_button_hold(int index);
#endif

#if defined(HAVE_LCD_SLEEP) && defined(HAVE_LCD_SLEEP_SETTING)
void lcd_set_sleep_after_backlight_off(int index);
#endif

#else /* !HAVE_BACKLIGHT */
#define backlight_init()
#endif /* !HAVE_BACKLIGHT */

#ifdef HAVE_REMOTE_LCD
void remote_backlight_on(void);
void remote_backlight_off(void);
void remote_backlight_set_timeout(int value);
void remote_backlight_set_timeout_plugged(int value);
bool is_remote_backlight_on(bool ignore_always_off);
int remote_backlight_get_current_timeout(void);

#ifdef HAS_REMOTE_BUTTON_HOLD
void remote_backlight_hold_changed(bool rc_hold_button);
void remote_backlight_set_on_button_hold(int index);
#endif
#endif /* HAVE_REMOTE_LCD */

#ifdef SIMULATOR
void sim_backlight(int value);
void sim_remote_backlight(int value);
#endif

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
#ifdef BACKLIGHT_FULL_INIT
extern int backlight_brightness;
#else
#define backlight_brightness DEFAULT_BRIGHTNESS_SETTING
#endif
void backlight_set_brightness(int val);
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_set_brightness(int val);
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTON_LIGHT
void buttonlight_on(void);
void buttonlight_off(void);
void buttonlight_set_timeout(int value);
#endif

/* Private API for use in target tree backlight code only */
#ifdef HAVE_BUTTON_LIGHT
int  buttonlight_get_current_timeout(void);
#endif

#endif /* BACKLIGHT_H */
