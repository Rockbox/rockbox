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

static int brightness = DEFAULT_BRIGHTNESS_SETTING;
static const unsigned short log_brightness[] = {
    0x2710, 0x27ac, 0x2849, 0x2983, 0x2abd, 0x2c93, 0x2e6a, 0x30dd,
    0x3351, 0x3661, 0x3971, 0x3d1f, 0x40cc, 0x4516, 0x4960, 0x4e47,
    0x532e, 0x58b1, 0x5e35, 0x6456, 0x6a76, 0x7134, 0x77f1, 0x7f4c,
    0x86a6, 0x8e9d, 0x9695, 0x9f29, 0xa7bd, 0xb0ee, 0xba1f, 0xc350
};

bool _backlight_init(void)
{
    /* configure PD4 as output */
    GPIO_PDCON |= (1<<4);

    /* set PD4 low (backlight off) */
    GPIO_PDDR &= ~(1<<4);

    /* IOMUXB - set PWM0 pin as GPIO */
    SCU_IOMUXB_CON &= ~(1 << 11); /* type<<11<<channel */

    /* DIV/2, PWM reset */
    PWMT0_CTRL = (0<<9) | (1<<7);

    /* set pwm frequency to 500Hz - my lcd panel can't cope more reliably */
    /* (apb_freq/pwm_freq)/pwm_div = (50 000 000/500)/2 */
    PWMT0_LRC = 50000; 
    PWMT0_HRC = log_brightness[brightness];

    /* reset counter */
    PWMT0_CNTR = 0x00;
    
    /* DIV/2, PWM output enable, PWM timer enable */
    PWMT0_CTRL = (0<<9) | (1<<3) | (1<<0);

    return true;
}

void _backlight_on(void)
{
    /* enable PWM clock */
    SCU_CLKCFG &= ~(1<<29);

    /* set output pin as PWM pin */
    SCU_IOMUXB_CON |= (1<<11); /* type<<11<<channel */

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

void _backlight_set_brightness(int val)
{
    brightness = val & 0x1f;
    PWMT0_HRC = log_brightness[brightness];
}
