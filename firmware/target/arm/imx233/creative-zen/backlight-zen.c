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
#include "backlight.h"
#include "lcd.h"
#include "backlight-target.h"
#include "uartdbg-imx233.h"
#include "pinctrl-imx233.h"

void _backlight_set_brightness(int level)
{
#ifdef CREATIVE_ZENV
    lcd_set_contrast(level);
#else
    unsigned val = (level + 200) * level / 1000;
    if(level != 0)
    {
        for(unsigned mask = 0x10; mask; mask >>= 1)
            imx233_uartdbg_send((val & mask) ? 0xff : 0xf8);
        imx233_uartdbg_send(0);
        imx233_pinctrl_set_gpio(1, 12, true);
    }
    else
        imx233_pinctrl_set_gpio(1, 12, false);
#endif
}

bool _backlight_init(void)
{
#ifndef CREATIVE_ZENV
    imx233_pinctrl_acquire(1, 12, "backlight_enable");
    imx233_pinctrl_set_function(1, 12, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(1, 12, true);
    imx233_uartdbg_init(BAUD_38400);
#endif
    return true;
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    /* restore the previous backlight level */
    _backlight_set_brightness(backlight_brightness);
}

void _backlight_off(void)
{
    /* there is no real on/off but we can set to 0 brightness */
    _backlight_set_brightness(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}
