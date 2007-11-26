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

/*
 *  force the backlight on
 *  now enabled regardless of HAVE_BACKLIGHT because it is not needed to
 *  build and makes modded targets easier to update
 */
void backlight_force_on(struct plugin_api* rb)
{
    if(!rb) return;
/* #ifdef HAVE_BACKLIGHT */
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(0);
#if CONFIG_CHARGING
    if (rb->global_settings->backlight_timeout_plugged > 0)
        rb->backlight_set_timeout_plugged(0);
#endif /* CONFIG_CHARGING */
/* #endif */ /* HAVE_BACKLIGHT */
}    

/*
 *  reset backlight operation to its settings
 *  now enabled regardless of HAVE_BACKLIGHT because it is not needed to
 *  build and makes modded targets easier to update
 */
void backlight_use_settings(struct plugin_api* rb)
{
    if(!rb) return;
/* #ifdef HAVE_BACKLIGHT */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#if CONFIG_CHARGING
    rb->backlight_set_timeout_plugged(rb->global_settings-> \
                                                     backlight_timeout_plugged);
#endif /* CONFIG_CHARGING */
/* #endif */ /* HAVE_BACKLIGHT */
}
