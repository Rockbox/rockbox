/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 by Lorenzo Miori
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
#include <fcntl.h>
#include <stdio.h>
#include "unistd.h"

bool backlight_hw_init(void)
{
    /* We have nothing to do */
    return true;
}

void backlight_hw_on(void)
{
    FILE *f = fopen("/sys/power/state", "w");
    fputs("on", f);
    fclose(f);
    lcd_enable(true);
}

void backlight_hw_off(void)
{
    FILE * f;

    /* deny the player to sleep deep */
    f = fopen("/sys/power/wake_lock", "w");
    fputs("player", f);
    fclose(f);

    /* deny the player to mute */
    f = fopen("/sys/class/codec/wm8740_mute", "w");
    fputc(0, f);
    fclose(f);

    /* turn off backlight */
    f = fopen("/sys/power/state", "w");
    fputs("mem", f);
    fclose(f);

}

void backlight_hw_brightness(int brightness)
{
    /* Just another check... */
    if (brightness > MAX_BRIGHTNESS_SETTING)
        brightness = MAX_BRIGHTNESS_SETTING;
    if (brightness < MIN_BRIGHTNESS_SETTING)
        brightness = MIN_BRIGHTNESS_SETTING;

    FILE *f = fopen("/sys/devices/platform/rk29_backlight/backlight/rk28_bl/brightness", "w");
    fprintf(f, "%d", brightness);
    fclose(f);
}
