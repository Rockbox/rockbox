/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#include "system.h"
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "power.h"
#include "lcd-target.h"

static void _backlight_write_brightness(int brightness)
{
    /*
       Maps brightness int to percentage value found in OF

       OF   PWM1H
       5%   14
       10%  140
       15%  210
       20%  280
       ...
       95%  1330
       100% 1400
    */
    if (brightness > 20)
        brightness = 20;
    else if (brightness < 0)
        brightness = 0;

    IO_CLK_PWM1H = brightness*70;
}

void backlight_hw_on(void)
{
    if (!lcd_active())
    {
        lcd_awake();
        lcd_update();
    }

    /* set GIO34 as PWM1 */
    IO_GIO_FSEL3 = (IO_GIO_FSEL3 & 0xFFF3) | (1 << 2);

#if (CONFIG_BACKLIGHT_FADING == BACKLIGHT_NO_FADING)
    _backlight_write_brightness(backlight_brightness);
#endif
}

void backlight_hw_off(void)
{
    _backlight_write_brightness(0);

    bitclr16(&IO_GIO_FSEL3, 0xC); /* set GIO34 to normal GIO */
    bitclr16(&IO_GIO_INV2, (1 << 2)); /* make sure GIO34 is not inverted */
    IO_GIO_BITCLR2 = (1 << 2); /* drive GIO34 low */
}

/* Assumes that the backlight has been initialized */
void backlight_hw_brightness(int brightness)
{
    _backlight_write_brightness(brightness);
}

void __backlight_dim(bool dim_now)
{
    backlight_hw_brightness(dim_now ?
        DEFAULT_BRIGHTNESS_SETTING :
        DEFAULT_DIMNESS_SETTING);
}

bool backlight_hw_init(void)
{
    IO_CLK_PWM1C = 0x58D; /* as found in OF */

    backlight_hw_brightness(backlight_brightness);
    return true;
}

