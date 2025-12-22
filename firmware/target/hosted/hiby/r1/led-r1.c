/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Melissa Autumn
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

static const char * const sysfs_blue_led =
"/sys/class/leds/blue/brightness";
static const char * const sysfs_red_led =
"/sys/class/leds/red/brightness";
static const char * const sysfs_red_trigger =
"/sys/class/leds/red/trigger";

/* Ref: https://guide.hiby.com/en/docs/products/audio_player/hiby_r1/guide#led-indicator

Note: The red breathing effect seems to be just the "breathing" led trigger.
By default red is turned off and blue is set to 50 brightness.
*/
enum R1_LEDS {
    LED_INVALID = -1,
    LED_OFF = 0,
    LED_BLUE,
    LED_RED,
};

#define DEFAULT_BRIGHTNESS 50

int _last_brightness = DEFAULT_BRIGHTNESS;
int _last_led = LED_INVALID;

void _set_led(enum R1_LEDS led) {
    if (_last_led != led) {
        _last_led = led;

        if (_last_brightness == 0) {
            _last_brightness = DEFAULT_BRIGHTNESS;
        }

        switch(led) {
        case LED_OFF:
            sysfs_set_int(sysfs_blue_led, 0);
            sysfs_set_int(sysfs_red_led, 0);
            break;
        case LED_BLUE:
            sysfs_set_int(sysfs_blue_led, _last_brightness);
            sysfs_set_int(sysfs_red_led, 0);
            break;
        case LED_RED:
            sysfs_set_int(sysfs_blue_led, 0);
            sysfs_set_int(sysfs_red_led, _last_brightness);
            sysfs_set_string(sysfs_red_trigger, "breathing");
            break;
        default:
            break;
        }
    }
}

#ifdef HAVE_GENERAL_PURPOSE_LED
void led_hw_brightness(int brightness)
{
    if (brightness != _last_brightness) {
        _last_brightness = brightness;
        _set_led(_last_led);
    }
}

void led_hw_charged(void)
{
    if (global_settings.use_led_indicators) {
        _set_led(LED_BLUE);
    }
}

void led_hw_charging(void)
{
    _set_led(LED_RED);
}

void led_hw_off(void)
{
    // Don't turn off led if charging
    if (!charging_state()) {
        _set_led(LED_OFF);
    }
}

void led_hw_on(void)
{
    if (charging_state()) {
        led_hw_charging();
    } else {
        if (global_settings.use_led_indicators) {
            _set_led(LED_BLUE);
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

