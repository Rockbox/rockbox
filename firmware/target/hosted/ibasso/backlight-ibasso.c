/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include "debug.h"
#include "lcd.h"
#include "panic.h"

#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


/*
    Prevent excessive backlight_hw_on usage.
    Required for proper seeking.
*/
static bool _backlight_enabled = false;


bool backlight_hw_init(void)
{
    TRACE;

    /*
        /sys/devices/platform/rk29_backlight/backlight/rk28_bl/bl_power
        0: backlight on
    */
    if(! sysfs_set_int(SYSFS_BACKLIGHT_POWER, 0))
    {
        DEBUGF("ERROR %s: Can not enable backlight.", __func__);
        panicf("ERROR %s: Can not enable backlight.", __func__);
        return false;
    }

    _backlight_enabled = true;

    return true;
}


void backlight_hw_on(void)
{
    if(! _backlight_enabled)
    {
        backlight_hw_init();
        lcd_enable(true);
    }
}


void backlight_hw_off(void)
{
    TRACE;

    /*
        /sys/devices/platform/rk29_backlight/backlight/rk28_bl/bl_power
        1: backlight off
    */
    if(! sysfs_set_int(SYSFS_BACKLIGHT_POWER, 1))
    {
        DEBUGF("ERROR %s: Can not disable backlight.", __func__);
        return;
    }

    lcd_enable(false);

    _backlight_enabled = false;
}


/*
    Prevent excessive backlight_hw_brightness usage.
    Required for proper seeking.
*/
static int _current_brightness = -1;


void backlight_hw_brightness(int brightness)
{
    if(brightness > MAX_BRIGHTNESS_SETTING)
    {
        DEBUGF("DEBUG %s: Adjusting brightness from %d to MAX.", __func__, brightness);
        brightness = MAX_BRIGHTNESS_SETTING;
    }
    if(brightness < MIN_BRIGHTNESS_SETTING)
    {
        DEBUGF("DEBUG %s: Adjusting brightness from %d to MIN.", __func__, brightness);
        brightness = MIN_BRIGHTNESS_SETTING;
    }

    if(_current_brightness == brightness)
    {
        return;
    }

    TRACE;

    _current_brightness = brightness;

    /*
        /sys/devices/platform/rk29_backlight/backlight/rk28_bl/max_brightness
        0 ... 255
    */
    if(! sysfs_set_int(SYSFS_BACKLIGHT_BRIGHTNESS, _current_brightness))
    {
        DEBUGF("ERROR %s: Can not set brightness.", __func__);
        return;
    }
}
