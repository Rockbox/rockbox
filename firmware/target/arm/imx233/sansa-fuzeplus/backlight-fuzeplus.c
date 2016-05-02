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
#include "dpc-imx233.h"

/* see comments in next function */
void backlight_hw_brightness_pulse(struct imx233_dpc_ctx_t *ctx, intptr_t user)
{
    (void) ctx;
    int brightness = user;
    while(brightness-- > 0)
    {
        imx233_pinctrl_set_gpio(1, 28, false);
        imx233_pinctrl_set_gpio(1, 28, true);
    }
}

void backlight_hw_brightness(int brightness)
{
    if(brightness != 0)
        brightness = MAX_BRIGHTNESS_SETTING + 1 - brightness;
    imx233_pinctrl_set_gpio(1, 28, false);
    /* The 600us delay is very long, especially when fading where the code will
     * repeat a lot of those without yielding. Using a DPC we can achieve good
     * accuracy (the timing is important) without blocking the entire system.
     * However this function may be called with IRQ disabled, for example in
     * panic(). In this case, we just a udelay. */
    if(!irq_enabled())
    {
        udelay(600);
        backlight_hw_brightness_pulse(NULL, brightness);
    }
    else
        imx233_dpc_start(true, 600, backlight_hw_brightness_pulse, brightness);
}

bool backlight_hw_init(void)
{
    imx233_pinctrl_acquire(1, 28, "backlight");
    imx233_pinctrl_set_function(1, 28, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(1, 28, PINCTRL_DRIVE_8mA);
    imx233_pinctrl_enable_gpio(1, 28, true);
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
