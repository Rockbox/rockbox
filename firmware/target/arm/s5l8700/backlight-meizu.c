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

void backlight_hw_brightness(int brightness)
{
    /* pwm = round(sqrt(2)**x), where brightness level x = 1..16 */
    static const unsigned int logtable[] =
        {1, 2, 3, 4, 6, 8, 11, 16, 23, 32, 45, 64, 91, 128, 181, 256};

    /* set PWM width */
    TCDATA0 = logtable[brightness];
}

void backlight_hw_on(void)
{
    /* configure backlight pin P0.2 as timer PWM output */
    PCON0 = ((PCON0 & ~(3 << 4)) | (2 << 4));
    backlight_hw_brightness(backlight_brightness);
}

void backlight_hw_off(void)
{
    /* configure backlight pin P0.2 as GPIO and switch off */
    PCON0 = ((PCON0 & ~(3 << 4)) | (1 << 4));
    PDAT0 &= ~(1 << 2);
}

bool backlight_hw_init(void)
{
    /* enable timer clock */
    PWRCON &= ~(1 << 4);

    /* configure timer */
    TCCMD = (1 << 1);   /* TC_CLR */
    TCCON = (0 << 13) | /* TC_INT1_EN */
            (0 << 12) | /* TC_INT0_EN */
            (1 << 11) | /* TC_START */
            (3 << 8) |  /* TC_CS = PCLK / 64 */
            (1 << 4);   /* TC_MODE_SEL = PWM mode */
    TCDATA1 = 256;      /* set PWM period */
    TCPRE = 20;         /* prescaler */
    TCCMD = (1 << 0);   /* TC_EN */
   
    backlight_hw_on();

    return true;
}

