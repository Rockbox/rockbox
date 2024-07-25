/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Jonas Haggqvist
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
#include "lib/mul_id3.h" /* collect_dir_stats & display_dir_stats */

enum plugin_status plugin_start(const void* parameter)
{    
    (void)parameter;
    int button, success;
    static struct dir_stats stats;
    stats.dirname[0] = '/';
    stats.count_all = true;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    display_dir_stats(&stats);
    success = collect_dir_stats(&stats, NULL);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    if (!success)
        return PLUGIN_OK;

    display_dir_stats(&stats);
#ifdef HAVE_BACKLIGHT
#ifdef HAVE_REMOTE_LCD
    rb->remote_backlight_on();
#endif
    rb->backlight_on();
#endif
    rb->splash(HZ, "Done");
    display_dir_stats(&stats);
    while (1) {
        switch (button = rb->get_action(CONTEXT_STD, TIMEOUT_BLOCK))
        {
            case ACTION_STD_CANCEL:
                return PLUGIN_OK;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
        }
    }
    return PLUGIN_OK;
}
