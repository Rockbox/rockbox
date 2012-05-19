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
#include "pwm-imx233.h"

void _backlight_set_brightness(int brightness)
{
    imx233_pwm_setup_channel(4, 1024, HW_PWM_PERIODx__CDIV__DIV_1,
        0, HW_PWM_PERIODx__STATE__HIGH,
        (brightness * 1024) / 100, HW_PWM_PERIODx__STATE__LOW);
    imx233_pwm_enable_channel(4, true);
}

bool _backlight_init(void)
{
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
