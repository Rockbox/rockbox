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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "helper.h"

/*  Force the backlight on */
void backlight_force_on(const struct plugin_api* rb)
{
    if(!rb)
        return;
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(0);
#if CONFIG_CHARGING
    if (rb->global_settings->backlight_timeout_plugged > 0)
        rb->backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
}

/*  Reset backlight operation to its settings */
void backlight_use_settings(const struct plugin_api* rb)
{
    if (!rb)
        return;
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#if CONFIG_CHARGING
    rb->backlight_set_timeout_plugged(rb->global_settings->
                                      backlight_timeout_plugged);
#endif /* CONFIG_CHARGING */
}

#ifdef HAVE_REMOTE_LCD
/*  Force the backlight on */
void remote_backlight_force_on(const struct plugin_api* rb)
{
    if (!rb)
        return;
    if (rb->global_settings->remote_backlight_timeout > 0)
        rb->remote_backlight_set_timeout(0);
#if CONFIG_CHARGING
    if (rb->global_settings->remote_backlight_timeout_plugged > 0)
        rb->remote_backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
}

/*  Reset backlight operation to its settings */
void remote_backlight_use_settings(const struct plugin_api* rb)
{
    if (!rb)
        return;
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
void buttonlight_force_on(const struct plugin_api* rb)
{
    if (!rb)
        return;
    if (rb->global_settings->buttonlight_timeout > 0)
        rb->buttonlight_set_timeout(0);
}

/*  Reset buttonlight operation to its settings */
void buttonlight_use_settings(const struct plugin_api* rb)
{
    if (!rb)
        return;
    rb->buttonlight_set_timeout(rb->global_settings->buttonlight_timeout);
}
#endif /* HAVE_BUTTON_LIGHT */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void backlight_brightness_set(const struct plugin_api *rb,
                              int brightness)
{
    if (!rb)
        return;
    rb->backlight_set_brightness(brightness);
}

void backlight_brightness_use_setting(const struct plugin_api *rb)
{
    if (!rb)
        return;
    rb->backlight_set_brightness(rb->global_settings->brightness);
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */
