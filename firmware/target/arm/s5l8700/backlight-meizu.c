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
    Interrupt-driven backlight driver using the PWM mode of a hardware timer.
    
    Backlight brightness is implemented by configuring one of the timers in
    the SoC for PWM mode. In this mode, two interrupts are generated for each
    cycle, one at the start of the cycle and another one sometime between the
    first interrupt and the start of the next cycle. The backlight is switched
    on at the first interrupt and switched off at the second interrupt. This
    way, the position in time of the second interrupt determines the duty cycle
    and thereby the brightness of the backlight.
    The backlight is switched on and off by means of a GPIO pin.
 */

void INT_TIMERA(void)
{
    unsigned int tacon = TACON;
    
    /* clear interrupts */
    TACON = tacon;

    /* TA_INT1, start of PWM cycle: enable backlight */
    if (tacon & (1 << 17)) {
        PDAT0 |= (1 << 2);
    }
    
    /* TA_INT0, disable backlight until next cycle */
    if (tacon & (1 << 16)) {
        PDAT0 &= ~(1 << 2);
    }
}

void _backlight_set_brightness(int brightness)
{
    static const unsigned char logtable[] = {0, 1, 2, 3, 5, 7, 10, 15, 22, 31, 44, 63, 90, 127, 180, 255};

    if (brightness == MIN_BRIGHTNESS_SETTING) {
        /* turn backlight fully off and disable interrupt */
        PDAT0 &= ~(1 << 2);
        INTMSK &= ~(1 << 5);
    }
    else if (brightness == MAX_BRIGHTNESS_SETTING) {
        /* turn backlight fully on and disable interrupt */
        PDAT0 |= (1 << 2);
        INTMSK &= ~(1 << 5);
    }
    else {
        /* set PWM width and enable interrupt */
        TADATA0 = logtable[brightness];
        INTMSK |= (1 << 5);
    }
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
    /* enable backlight pin as GPIO */
    PCON0 = ((PCON0 & ~(3 << 4)) | (1 << 4));

    /* enable timer clock */
    PWRCON &= ~(1 << 4);

    /* configure timer */
    TACMD = (1 << 1);   /* TA_CLR */
    TACMD = (1 << 0);   /* TA_EN */
    TACON = (1 << 13) | /* TA_INT1_EN */
            (1 << 12) | /* TA_INT0_EN */
            (1 << 11) | /* TA_START */
            (3 << 8) |  /* TA_CS = PCLK / 64 */
            (1 << 4);   /* TA_MODE_SEL = PWM mode */
    TADATA1 = 255;   /* set PWM period */
    TAPRE = 30;    /* prescaler */
   
    _backlight_on();

    return true;
}

