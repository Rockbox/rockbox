/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "pwm-imx233.h"
#include "pinctrl-imx233.h"
#include "kernel.h"

static int last_brightness;

void backlight_hw_brightness(int brightness)
{
    bool en = brightness > 0;
    /* brightness has extreme dip above 88% duty cycle, limit range to [87%, 0%] */
    if (en)
        imx233_pwm_setup_simple(2, 24000, 88 - brightness);
    imx233_pwm_enable(2, en);
    if (last_brightness == 0 && en)
        sleep(HZ / 25); /* make sure PWM settings are applied */
    last_brightness = brightness;
    imx233_pinctrl_set_gpio(0, 10, en);
}

bool backlight_hw_init(void)
{
    imx233_pinctrl_acquire(0, 10, "backlight_enable");
    imx233_pinctrl_set_function(0, 10, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 10, true);
#ifndef BOOTLOADER
    backlight_hw_brightness(0);
    return false;
#else
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
#endif
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    sleep(HZ / 10); /* make sure screen is not white anymore */
    /* don't do anything special, the core will set the brightness */
}

void backlight_hw_off(void)
{
    /* there is no real on/off but we can set to 0 brightness */
    backlight_hw_brightness(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}
