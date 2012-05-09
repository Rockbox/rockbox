/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "lcd.h"
#include "backlight.h"
#include "backlight-target.h"
#include "pinctrl-imx233.h"

void _backlight_set_brightness(int brightness)
{
    if(brightness != 0)
        brightness = 32 - (brightness * 32) / 100;
    imx233_set_gpio_output(1, 28, false);
    udelay(600);
    while(brightness-- > 0)
    {
        imx233_set_gpio_output(1, 28, false);
        imx233_set_gpio_output(1, 28, true);
    }
}

bool _backlight_init(void)
{
    imx233_pinctrl_acquire_pin(1, 28, "backlight");
    imx233_set_pin_function(1, 28, PINCTRL_FUNCTION_GPIO);
    imx233_set_pin_drive_strength(1, 28, PINCTRL_DRIVE_8mA);
    imx233_enable_gpio_output(1, 28, true);
    _backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    /* don't do anything special, the core will set the brightness */
}

void _backlight_off(void)
{
    /* there is no real on/off but we can set to 0 brightness */
    _backlight_set_brightness(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}
