/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "mpr121-zenxfi3.h"

void backlight_hw_brightness(int brightness)
{
    imx233_pwm_setup_simple(2, 24000, brightness);
    imx233_pwm_enable(2, true);
}

bool backlight_hw_init(void)
{
    backlight_hw_brightness(DEFAULT_BRIGHTNESS_SETTING);
    return true;
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
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

/* ELE8 is the button light GPIO */
void buttonlight_hw_on(void)
{
    /* assume mpr121 was initialized by button-zenxfi3.c */
    mpr121_set_gpio_output(8, ELE_GPIO_SET);
}

void buttonlight_hw_off(void)
{
    /* assume mpr121 was initialized by button-zenxfi3.c */
    mpr121_set_gpio_output(8, ELE_GPIO_CLR);
}

void buttonlight_hw_brightness(int brightness)
{
    /* assume mpr121 was initialized by button-zenxfi3.c */
    /* since backlight brightness is the same for the screen and the button light,
     * map [MIN_BRIGHTNESS_SETTING,MAX_BRIGHTNESS_SETTING] to
     * [ELE_PWM_MIN_DUTY,ELE_PWM_MAX_DUTY] */
    brightness = ELE_PWM_MIN_DUTY + (brightness - MIN_BRIGHTNESS_SETTING) *
        (ELE_PWM_MAX_DUTY - ELE_PWM_MIN_DUTY + 1) /
        (MAX_BRIGHTNESS_SETTING - MIN_BRIGHTNESS_SETTING + 1);
    brightness = MIN(brightness, ELE_PWM_MAX_DUTY);
    mpr121_set_gpio_pwm(8, brightness);
}
