/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Mark Arigo
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
#include "backlight-target.h"

#define MIN_BRIGHTNESS 0x80ff08ff

static const int log_brightness[12] = {0,4,8,12,20,28,40,60,88,124,176,255};

/* Returns the current state of the backlight (true=ON, false=OFF). */
bool _backlight_init(void)
{
    return (GPO32_ENABLE & 0x1000000) ? true : false;
}

void _backlight_hw_on(void)
{
    GPO32_ENABLE |= 0x1000000;
}

void _backlight_hw_off(void)
{
    GPO32_ENABLE &= ~0x1000000;
}

void _buttonlight_set_brightness(int brightness)
{
    /* clamp the brightness value */
    brightness = MAX(0, MIN(15, brightness));

    outl(MIN_BRIGHTNESS-(log_brightness[brightness - 1] << 16), 0x7000a010);
}

void _buttonlight_on(void)
{
    /* turn on all touchpad leds */
    GPIOA_OUTPUT_VAL |= BUTTONLIGHT_ALL;
}

void _buttonlight_off(void)
{
    /* turn off all touchpad leds */
    GPIOA_OUTPUT_VAL &= ~BUTTONLIGHT_ALL;
}
