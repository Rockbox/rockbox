/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Dave Chapman
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
#include "kernel.h"
#include "backlight.h"
#include "backlight-target.h"
#include "pmu-target.h"

#ifdef HAVE_LCD_SLEEP
#include "lcd.h"
#include "lcd-s5l8702.h"
#endif

static unsigned char led_out = DEFAULT_BRIGHTNESS_SETTING << 2;
static bool led_on;

static void led_update(void)
{
    pmu_set_backlight(led_out, led_on);
}

void backlight_hw_brightness(int brightness)
{
    led_out = brightness << 2;
    led_update();
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_SLEEP
    if (!lcd_active())
        lcd_awake();
#endif
    led_on = true;
    led_update();
}

void backlight_hw_off(void)
{
    led_on = false;
    led_update();
}

bool backlight_hw_init(void)
{
    led_out = DEFAULT_BRIGHTNESS_SETTING << 2;
    backlight_hw_on();
    return true;
}

/* Kill the backlight, instantly. */
void backlight_hw_kill(void)
{
    backlight_hw_off();
}
