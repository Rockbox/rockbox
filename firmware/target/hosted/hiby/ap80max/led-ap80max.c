/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Melissa Autumn
 * Copyright (C) 2026 by Jerry Shen
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "backlight-target.h"
#include "sysfs.h"
#include "panic.h"
#include "lcd.h"
#include "settings.h"
#include "power.h"

/* Same sgm31324 controller as the R3 Pro II, so the same sysfs node */
static const char * const sysfs_led_type =
"/sys/class/leds/sgm31324-leds/led_pattern";

/* FIXME: taken from the R3 Pro II's leds_sgm31324_add.sh; this player ships
   its own copy and the ordering has not been checked against it */
enum AP80MAX_LED_PATTERNS {
    LED_PATTERN_INVALID = -1,
    LED_PATTERN_OFF = 0,
    LED_PATTERN_LIGHT_BLUE,
    LED_PATTERN_YELLOW,
    LED_PATTERN_CYAN,
    LED_PATTERN_ORANGE,
    LED_PATTERN_UNK_1, /* This might do something, but it seems to just be off */
    LED_PATTERN_NORMAL_RED_BLINK,
    LED_PATTERN_FAST_RED_BLINK,
    LED_PATTERN_GREEN,
    LED_PATTERN_BLUE, /* A slightly more blue blue */
    LED_PATTERN_PINK,
    LED_PATTERN_WHITE,
};

int _last_buttonlight = LED_PATTERN_INVALID;

void _set_pattern(enum AP80MAX_LED_PATTERNS pattern) {
    if (_last_buttonlight != pattern) {
        _last_buttonlight = pattern;
        sysfs_set_int(sysfs_led_type, _last_buttonlight);
    }
}

#ifdef HAVE_GENERAL_PURPOSE_LED
void led_hw_brightness(int brightness)
{
    (void)brightness;
}

void led_hw_charged(void)
{
    if (global_settings.use_led_indicators) {
        _set_pattern(LED_PATTERN_GREEN);
    }
}

void led_hw_charging(void)
{
    _set_pattern(LED_PATTERN_NORMAL_RED_BLINK);
}

void led_hw_off(void)
{
    // Don't turn the led off if charging.
    if (charging_state())
        return;

    _set_pattern(LED_PATTERN_OFF);
}

void led_hw_on(void)
{
    if (charging_state()) {
        led_hw_charging();
    } else {
        if (global_settings.use_led_indicators) {
            _set_pattern(LED_PATTERN_LIGHT_BLUE);
        }
        else {
            led_hw_off();
        }
    }
}

void led_hw_set_mode(bool mode)
{
    if (mode) {
        led_hw_on();
    }
    else {
        led_hw_off();
    }
}
#endif
