/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Lorenzo Miori
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
#include "config.h"
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"
#include "lcd.h"
#include "lcd-target.h"
#include "pmu-ypr1.h"

static bool backlight_on_status = true; /* Is on or off? */

bool _backlight_init(void)
{
    /* We have nothing to do */
    return true;
}

void _backlight_on(void)
{
    if (!backlight_on_status) {
        _backlight_set_brightness(backlight_brightness);
    }

    backlight_on_status = true;

}

void _backlight_off(void)
{
    if (backlight_on_status) {
        _backlight_set_brightness(0);
    }

    backlight_on_status = false;
}

void _backlight_set_brightness(int brightness)
{
    /* Just another check... */
    if (brightness > MAX_BRIGHTNESS_SETTING) {
        brightness = MAX_BRIGHTNESS_SETTING;
    }
    if (brightness < 0) {
        brightness = MIN_BRIGHTNESS_SETTING;
    }
    /* Do the appropriate ioctl on the linux module */
    pmu_ioctl(MAX8819_IOCTL_LCD_DIM_CTRL, &brightness);
}
