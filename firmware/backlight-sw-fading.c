/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Thomas Martitz
 * Copyright (C) 2008 by Martin Ritter
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
#include "backlight-target.h"
#include "config.h"
#include "system.h"
#include "backlight.h"
#include "backlight-sw-fading.h"

/* To adapt a target do:
 * - make sure _backlight_on doesn't set the brightness to something other than
 *  the previous value (lowest brightness in most cases)
 * - #define USE_BACKLIGHT_SW_FADING in config-<target>.h
 */

/* can be MIN_BRIGHTNESS_SETTING-1 */
static int current_brightness = DEFAULT_BRIGHTNESS_SETTING;

void _backlight_fade_update_state(int brightness)
{
    current_brightness = brightness;
}

/* returns true if fade is finished */
static bool _backlight_fade_up(void)
{
    if (LIKELY(current_brightness < backlight_brightness))
    {
        _backlight_set_brightness(++current_brightness);
    }
    return(current_brightness >= backlight_brightness);
}

/* returns true if fade is finished */
static bool _backlight_fade_down(void)
{
    if (LIKELY(current_brightness > MIN_BRIGHTNESS_SETTING))
    {
        _backlight_set_brightness(--current_brightness);
        return false;
    }
    else
    {
        /* decrement once more, since backlight is off */
        current_brightness--;
        _backlight_off();
        return true;
    }
}

bool _backlight_fade_step(int direction)
{
    bool done;
    switch(direction)
    {
        case FADING_UP:
            done = _backlight_fade_up();
        break;
        case FADING_DOWN:
            done = _backlight_fade_down();
        break;
        default:
            done = true;
        break;
    }
    return(done);
}
