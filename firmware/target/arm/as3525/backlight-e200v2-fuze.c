/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "backlight-target.h"
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "ascodec-target.h"
#include "as3514.h"

static unsigned short backlight_brightness = DEFAULT_BRIGHTNESS_SETTING;

void _backlight_set_brightness(int brightness)
{
    backlight_brightness = brightness;

    if (brightness > 0)
        _backlight_on();
    else
        _backlight_off();
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_SLEEP
    backlight_lcd_sleep_countdown(false); /* stop counter */
#endif
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    ascodec_write(AS3514_DCDC15, backlight_brightness);
}

void _backlight_off(void)
{
    ascodec_write(AS3514_DCDC15, 0x0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
#ifdef HAVE_LCD_SLEEP
    backlight_lcd_sleep_countdown(true); /* start countdown */
#endif
}

void _buttonlight_on(void)
{
    GPIOD_PIN(7) = (1<<7);
    GPIOD_DIR |= (1<<7);
}

void _buttonlight_off(void)
{
    GPIOD_PIN(7) = 0;
    GPIOD_DIR |= (1<<7);
}
