/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
 * Copyright (C) 2025 by Melissa Autumn
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
#include <linux/input.h>

#include "sysfs.h"
#include "button.h"
#include "button-target.h"
#include "hibylinux_codec.h"
#include "touchscreen.h"
#ifdef HAVE_BACKLIGHT
#include "backlight.h"
#endif /* HAVE_BACKLIGHT */
/*
 *   /dev/input/event0: gpios (power/next)
 *   /dev/input/event1: touchscreen
 *   /dev/input/event2: adc (prev/play/left/right)
 *   /dev/input/event3: ????
 */


int button_map(int keycode)
{
    switch(keycode)
    {
        case KEY_VOLUMEDOWN:
            return BUTTON_VOL_DOWN;
        case KEY_VOLUMEUP:
            return BUTTON_VOL_UP;
        case KEY_PLAYPAUSE:
            return BUTTON_PLAY;
        case KEY_NEXTSONG:
            return BUTTON_NEXT;
        case KEY_PREVIOUSSONG:
            return BUTTON_PREV;
        case KEY_POWER:
            return BUTTON_POWER;
        case BTN_TOUCH:
        {
#ifdef HAVE_BACKLIGHT
            if (is_backlight_on(true)) {
                return BUTTON_TOUCH;
            }
            // Ignore
            return 0;
#else
            return BUTTON_TOUCH
#endif
        }
        default:
            return 0;
    }
}

bool headphones_inserted(void)
{
    #ifdef BOOTLOADER
    int ps = 0;
    #else
    int ps = hiby_get_outputs();
    #endif

    return (ps == 2 || ps == 3);
}
