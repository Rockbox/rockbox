/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: helper.c 12179 2007-08-14 23:08:15Z peter $
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

/* the plugin must declare the plugin_api struct pointer itself */
extern struct plugin_api* rb;

/* force the backlight on */
void backlight_force_on(void)
{
#ifdef HAVE_BACKLIGHT
    rb->backlight_set_timeout(1);
#if CONFIG_CHARGING
    rb->backlight_set_timeout_plugged(1);
#endif
#endif
}    

/* reset backlight operation to its settings */
void backlight_use_settings(void)
{
#ifdef HAVE_BACKLIGHT
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#if CONFIG_CHARGING
    rb->backlight_set_timeout_plugged(rb->global_settings-> \
                                                     backlight_timeout_plugged);
#endif
#endif
}
