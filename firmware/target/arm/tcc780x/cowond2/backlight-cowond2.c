/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "backlight-target.h"
#include "system.h"
#include "backlight.h"
#include "pcf50606.h"
#include "tcc780x.h"

static unsigned short backlight_brightness = DEFAULT_BRIGHTNESS_SETTING;

int _backlight_init(void)
{
    _backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

void _backlight_set_brightness(int brightness)
{
    backlight_brightness = brightness;

    int level = disable_irq_save();
    pcf50606_write(PCF5060X_PWMC1, 0xe1 | (14-backlight_brightness)<<1);
    pcf50606_write(PCF5060X_GPOC1, 0x3);
    restore_irq(level);
    
    if (brightness > 0)
        _backlight_on();
    else
        _backlight_off();
}

void _backlight_on(void)
{
    GPIOA_SET = (1<<6);
}

void _backlight_off(void)
{
    GPIOA_CLEAR = (1<<6);
}
