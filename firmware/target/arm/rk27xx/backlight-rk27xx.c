/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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

bool _backlight_init(void)
{
    /* configure PD4 as output */
    GPIO_PDCON |= (1<<4);

    /* set PD4 low (backlight off) */
    GPIO_PDDR &= ~(1<<4);

    /* IOMUXB - set PWM0 pin as GPIO */
    SCU_IOMUXB_CON &= ~(1 << 11); /* type<<11<<channel */

    /* setup pwm */
    PWMT0_CTRL = (0<<9) | (1<<7);

    /* set pwm frequency ~10kHz */
    /* (apb_freq/pwm_freq)/pwm_div = (50 000 000/10 000)/2 */
    PWMT0_LRC = 2500; 
    PWMT0_HRC = 1;

    PWMT0_CNTR = 0x00;
    PWMT0_CTRL = (0<<9) | (1<<3) | (1<<0);

    return true;
}

void _backlight_on(void)
{
    /* enable PWM clock */
    SCU_CLKCFG &= ~(1<<29);

    /* set output pin as PWM pin */
    SCU_IOMUXB_CON |= (1<<11); /* type<<11<<channel */

    /* 100% duty cycle. Other settings doesn't work for
     * unknown reason
     */
    PWMT0_HRC = PWMT0_LRC;

    /* pwm enable */
    PWMT0_CTRL |= (1<<3) | (1<<0);
}

void _backlight_off(void)
{
    /* setup PWM0 pin as GPIO which is pulled low */
    SCU_IOMUXB_CON &= ~(1<<11);

    /* stop pwm timer */
    PWMT0_CTRL &= ~(1<<3) | (1<<0);

    /* disable PWM clock */
    SCU_CLKCFG |= (1<<29);
}

void _backlight_set_brightness(int brightness)
{
    (void)brightness;
    /* doesn't work for unknown reason */
}
