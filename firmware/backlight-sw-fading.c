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
#include "config.h"
#include "backlight-target.h"
#include "system.h"
#include "backlight.h"
#include "backlight-sw-fading.h"

#ifndef BRIGHTNESS_STEP
#define BRIGHTNESS_STEP 1
#endif

/* To adapt a target do:
 * - make sure backlight_hw_on doesn't set the brightness to something other than
 *  the previous value (lowest brightness in most cases)
 * add proper #defines for software fading
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
#if BRIGHTNESS_STEP == 1
        backlight_hw_brightness(++current_brightness);
#else
        current_brightness += BRIGHTNESS_STEP;
        if (current_brightness > MAX_BRIGHTNESS_SETTING)
            current_brightness = MAX_BRIGHTNESS_SETTING;
	backlight_hw_brightness(current_brightness);
#endif
    }
    return(current_brightness >= backlight_brightness);
}

/* returns true if fade is finished */
static bool _backlight_fade_down(void)
{
    if (LIKELY(current_brightness > MIN_BRIGHTNESS_SETTING))
    {
#if BRIGHTNESS_STEP == 1
        backlight_hw_brightness(--current_brightness);
#else
        current_brightness -= BRIGHTNESS_STEP;
        if (current_brightness < MIN_BRIGHTNESS_SETTING)
            current_brightness = MIN_BRIGHTNESS_SETTING;
	backlight_hw_brightness(current_brightness);
#endif
        return false;
    }
    else
    {
        /* decrement once more, since backlight is off */
#if BRIGHTNESS_STEP == 1
        current_brightness--;
#else
        current_brightness=MIN_BRIGHTNESS_SETTING -1;
#endif
        backlight_hw_off();
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
