/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald, Dana Conrad
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

#include "backlight.h"
#include "backlight-target.h"
#include "lcd.h"
#include "pwm-x1000.h"

#define BL_LCD_CHN    0
#define BL_LCD_PERIOD 30000

static int backlight_calc_duty(int period, int min_duty, int brightness)
{
    return min_duty + (period - min_duty) * brightness / MAX_BRIGHTNESS_SETTING;
}

bool backlight_hw_init(void)
{
    pwm_init(BL_LCD_CHN);
    pwm_enable(BL_LCD_CHN);
    backlight_hw_brightness(MAX_BRIGHTNESS_SETTING);
    return true;
}

void backlight_hw_on(void)
{
    pwm_enable(BL_LCD_CHN);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true);
#endif
}

void backlight_hw_off(void)
{
    pwm_disable(BL_LCD_CHN);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false);
#endif
}

void backlight_hw_brightness(int brightness)
{
    int duty_ns = backlight_calc_duty(BL_LCD_PERIOD, 0, brightness);
    pwm_set_period(BL_LCD_CHN, BL_LCD_PERIOD, duty_ns);
}
