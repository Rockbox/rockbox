/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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
#include <stdbool.h>

#include "config.h"
#include "backlight.h"
#include "backlight-target.h"
#include "system.h"

/*
    Backlight driver using the PWM mode of a hardware timer.
    
    The PWM duty cycle depends exponentially on the configured brightness
    level. This makes the brightness curve more linear to the human eye.
 */

void _backlight_set_brightness(int brightness)
{
    /* pwm = (sqrt(2)**x)-1, where brightness level x = 0..16 */
    static const unsigned char logtable[] =
        {0, 1, 2, 3, 5, 7, 10, 15, 22, 31, 44, 63, 90, 127, 180, 255};

    /* set PWM width */
    TADATA0 = 255 - logtable[brightness];
}

void _backlight_on(void)
{
    _backlight_set_brightness(backlight_brightness);
}

void _backlight_off(void)
{
    _backlight_set_brightness(MIN_BRIGHTNESS_SETTING);
}

bool _backlight_init(void)
{
    /* enable backlight pin as timer output */
    PCON0 = ((PCON0 & ~(3 << 0)) | (2 << 0));

    /* enable timer clock */
    PWRCON &= ~(1 << 4);

    /* configure timer */
    TACMD = (1 << 1);   /* TC_CLR */
    TACON = (0 << 13) | /* TC_INT1_EN */
            (0 << 12) | /* TC_INT0_EN */
            (0 << 11) | /* TC_START */
            (3 << 8) |  /* TC_CS = PCLK / 64 */
            (1 << 4);   /* TC_MODE_SEL = PWM mode */
    TADATA1 = 255;      /* set PWM period */
    TAPRE = 20;         /* prescaler */
    TACMD = (1 << 0);   /* TC_EN */
   
    _backlight_on();

    return true;
}

