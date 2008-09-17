/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "jz4740.h"
#include "backlight-target.h"

#define GPIO_PWM          (32*3)+31
#define PWM_CHN            7
#define __gpio_as_PWM_CHN  __gpio_as_pwm7

static void set_backlight(int val)
{
    /* Taken from the OF */
    int tmp;
    tmp = (val/2 + __cpm_get_rtcclk()) / val;
    if(tmp > 0xFFFF)
        tmp = 0xFFFF;

    __tcu_set_half_data(PWM_CHN, (tmp * val * 1374389535) >> 5);
    __tcu_set_full_data(PWM_CHN, tmp);
}

static void set_backlight_on(void)
{
    __tcu_start_timer_clock(PWM_CHN);
    
    set_backlight(MAX_BRIGHTNESS_SETTING);

    __tcu_set_count(PWM_CHN, 0);
    __tcu_start_counter(PWM_CHN);

    __tcu_enable_pwm_output(PWM_CHN);
}

static void set_backlight_off(void)
{
    __tcu_stop_counter(PWM_CHN);
    __tcu_disable_pwm_output(PWM_CHN);
    __tcu_stop_timer_clock(PWM_CHN);
}

bool _backlight_init(void)
{
    __gpio_as_PWM_CHN();
    __tcu_start_timer_clock(PWM_CHN);

    __tcu_stop_counter(PWM_CHN);
    __tcu_disable_pwm_output(PWM_CHN);

    __tcu_init_pwm_output_low(PWM_CHN);
    __tcu_select_rtcclk(PWM_CHN);
    __tcu_select_clk_div1(PWM_CHN);

    __tcu_mask_half_match_irq(PWM_CHN);
    __tcu_mask_full_match_irq(PWM_CHN);

    set_backlight_on();

    return true;
}

void _backlight_on(void)
{
    set_backlight_on();
}

void _backlight_off(void)
{
    set_backlight_off();
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void _backlight_set_brightness(int brightness)
{
    set_backlight(brightness);
}
#endif
