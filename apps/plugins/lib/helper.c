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

#ifdef CPU_SH
/* Lookup table for using the BIT_N() macro in plugins */
const unsigned bit_n_table[32] = {
    1LU<< 0, 1LU<< 1, 1LU<< 2, 1LU<< 3,
    1LU<< 4, 1LU<< 5, 1LU<< 6, 1LU<< 7,
    1LU<< 8, 1LU<< 9, 1LU<<10, 1LU<<11,
    1LU<<12, 1LU<<13, 1LU<<14, 1LU<<15,
    1LU<<16, 1LU<<17, 1LU<<18, 1LU<<19,
    1LU<<20, 1LU<<21, 1LU<<22, 1LU<<23,
    1LU<<24, 1LU<<25, 1LU<<26, 1LU<<27,
    1LU<<28, 1LU<<29, 1LU<<30, 1LU<<31
};
#endif

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
