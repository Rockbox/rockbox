/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Peter D'Hoye
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

#include "plugin.h"
#include "helper.h"

int talk_val(long n, int unit, bool enqueue)
{
    #define NODECIMALS 0
    return rb->talk_value_decimal(n, unit, NODECIMALS, enqueue);
}

#ifdef HAVE_BACKLIGHT
/* Force the backlight on */
void backlight_force_on(void)
{
    rb->backlight_set_timeout(0);
#if CONFIG_CHARGING
    rb->backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
}

/* Turn off backlight timeout */
void backlight_ignore_timeout(void)
{
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(0);
#if CONFIG_CHARGING
    if (rb->global_settings->backlight_timeout_plugged > 0)
        rb->backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
}

/*  Reset backlight operation to its settings */
void backlight_use_settings(void)
{
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#if CONFIG_CHARGING
    rb->backlight_set_timeout_plugged(rb->global_settings->
                                      backlight_timeout_plugged);
#endif /* CONFIG_CHARGING */
}
#endif /* HAVE_BACKLIGHT */

#ifdef HAVE_SW_POWEROFF
static bool original_sw_poweroff_state = true;

void sw_poweroff_disable(void)
{
    original_sw_poweroff_state = rb->button_get_sw_poweroff_state();
    rb->button_set_sw_poweroff_state(false);
}

void sw_poweroff_restore(void)
{
    rb->button_set_sw_poweroff_state(original_sw_poweroff_state);
}
#endif

#ifdef HAVE_REMOTE_LCD
/*  Force the backlight on */
void remote_backlight_force_on(void)
{
    rb->remote_backlight_set_timeout(0);
#if CONFIG_CHARGING
    rb->remote_backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
}

/* Turn off backlight timeout */
void remote_backlight_ignore_timeout(void)
{
    if (rb->global_settings->remote_backlight_timeout > 0)
        rb->remote_backlight_set_timeout(0);
#if CONFIG_CHARGING
    if (rb->global_settings->remote_backlight_timeout_plugged > 0)
        rb->remote_backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
}

/*  Reset backlight operation to its settings */
void remote_backlight_use_settings(void)
{
    rb->remote_backlight_set_timeout(rb->global_settings->
                                     remote_backlight_timeout);
#if CONFIG_CHARGING
    rb->remote_backlight_set_timeout_plugged(rb->global_settings-> 
                                             remote_backlight_timeout_plugged);
#endif /* CONFIG_CHARGING */
}
#endif /* HAVE_REMOTE_LCD */

#ifdef HAVE_BUTTON_LIGHT
/*  Force the buttonlight on */
void buttonlight_force_on(void)
{
    rb->buttonlight_set_timeout(0);
}

/*  Force the buttonlight off */
void buttonlight_force_off(void)
{
    rb->buttonlight_set_timeout(-1);
}

/* Turn off backlight timeout */
void buttonlight_ignore_timeout(void)
{
    if (rb->global_settings->buttonlight_timeout > 0)
        rb->buttonlight_set_timeout(0);
}

/*  Reset buttonlight operation to its settings */
void buttonlight_use_settings(void)
{
    rb->buttonlight_set_timeout(rb->global_settings->buttonlight_timeout);
}
#endif /* HAVE_BUTTON_LIGHT */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_brightness_set(int brightness)
{
    rb->backlight_set_brightness(brightness);
}

void backlight_brightness_use_setting(void)
{
    rb->backlight_set_brightness(rb->global_settings->brightness);
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
void buttonlight_brightness_set(int brightness)
{
    rb->buttonlight_set_brightness(brightness);
}

void buttonlight_brightness_use_setting(void)
{
    rb->buttonlight_set_brightness(rb->global_settings->buttonlight_brightness);
}
#endif /* HAVE_BUTTONLIGHT_BRIGHTNESS */
