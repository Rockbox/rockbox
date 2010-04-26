/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"
#include "lcd.h"

static bool _backlight_on = false;
static int _brightness = DEFAULT_BRIGHTNESS_SETTING;

/* Returns the current state of the backlight (true=ON, false=OFF). */
bool _backlight_init(void)
{
    and_l(~(1<<28),&GPIO_OUT);
    or_l((1<<28),&GPIO_FUNCTION);
    or_l((1<<28),&GPIO_ENABLE);
    return true;
}

void _backlight_hw_on(void)
{

    if (_backlight_on)
        return;
   
    _backlight_set_brightness(_brightness);
    _backlight_on = true;

}

void _backlight_hw_off(void)
{
    /* GPIO28 low */
    and_l(~(1<<28),&GPIO_OUT);
    _backlight_on = false;
}

void _backlight_set_brightness(int val)
{
    unsigned char i;

    and_l(~(1<<28),&GPIO_OUT);
    sleep(4);

    for (i=0;i<val;i++)
    {
        or_l((1<<28),&GPIO_OUT);
        and_l(~(1<<28),&GPIO_OUT);
    }
    
    or_l((1<<28),&GPIO_OUT);

    _brightness = val;
}

void _remote_backlight_on(void)
{
    /* I don't have remote to play with */
}

void _remote_backlight_off(void)
{
    /* I don't have remote to play with */
}
