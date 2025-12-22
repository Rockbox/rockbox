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

static const char * const sysfs_led_type =
"/sys/class/leds/sgm31324-leds/led_pattern";

/* Ref: https://guide.hiby.com/en/docs/products/audio_player/hiby_r3proii/guide#led-indicator-light

   Defined in leds_sgm31324_add.sh (off is defined in driver)

   ...
   echo regs="00000000540001010706"  > sgm31324
   echo regs="000000004500380B0000"  > sgm31324
   echo regs="00000000540000060600"  > sgm31324
   echo regs="00000000450040040000"  > sgm31324
   echo regs="00000000000000000006"  > sgm31324
   echo regs="001E7D0042BB2F000006"  > sgm31324
   echo regs="000E7D0042663F000006"  > sgm31324
   echo regs="00000000440000050006"  > sgm31324
   echo regs="00000000540000000D06"  > sgm31324
   echo regs="0000000051001C000506"  > sgm31324
   echo regs="00000000000000000006"  > sgm31324
   ...

*/
enum R3PROII_LED_PATTERNS {
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

void _set_pattern(enum R3PROII_LED_PATTERNS pattern) {
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
	// Don't turn off led if charging
	if (_last_buttonlight != LED_PATTERN_NORMAL_RED_BLINK) {
		_set_pattern(LED_PATTERN_OFF);
    }
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

