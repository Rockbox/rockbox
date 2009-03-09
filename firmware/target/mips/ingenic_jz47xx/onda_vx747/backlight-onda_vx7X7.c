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

/* PWM_CHN7 == GPIO(32*3 + 31) */
#define BACKLIGHT_GPIO  (32*3+31)
#define BACKLIGHT_PWM   7

#define SW_PWM 1

#if SW_PWM

static bool backlight_on;

static void set_backlight(int val)
{
    (void)val;
}

bool _backlight_init(void)
{
    __gpio_as_output(BACKLIGHT_GPIO);
    __gpio_set_pin(BACKLIGHT_GPIO);
    
    backlight_on = true;

    return true;
}

bool backlight_enabled(void)
{
    return backlight_on;
}

void _backlight_on(void)
{
    __gpio_set_pin(BACKLIGHT_GPIO);
    backlight_on = true;
}

void _backlight_off(void)
{
    __gpio_clear_pin(BACKLIGHT_GPIO);
    backlight_on = false;
}

#else

static int old_val;
static void set_backlight(int val)
{
    if(val == old_val)
        return;
    
    /* Taken from the OF */
    int tmp;
    tmp = (val/2 + __cpm_get_rtcclk()) / val;
    if(tmp > 0xFFFF)
        tmp = 0xFFFF;

    __tcu_set_half_data(BACKLIGHT_PWM, (tmp * val * 1374389535) >> 5);
    __tcu_set_full_data(BACKLIGHT_PWM, tmp);
    
    old_val = val;
}

static void set_backlight_on(void)
{
    if(old_val == MAX_BRIGHTNESS_SETTING)
        return;
    
    __tcu_start_timer_clock(BACKLIGHT_PWM);
    
    set_backlight(MAX_BRIGHTNESS_SETTING);

    __tcu_set_count(BACKLIGHT_PWM, 0);
    __tcu_start_counter(BACKLIGHT_PWM);

    __tcu_enable_pwm_output(BACKLIGHT_PWM);
}

static void set_backlight_off(void)
{
    __tcu_stop_counter(BACKLIGHT_PWM);
    __tcu_disable_pwm_output(BACKLIGHT_PWM);
    __tcu_stop_timer_clock(BACKLIGHT_PWM);
    
    old_val = -1;
}

bool _backlight_init(void)
{
    __gpio_as_pwm(BACKLIGHT_PWM);
    __tcu_start_timer_clock(BACKLIGHT_PWM);

    __tcu_stop_counter(BACKLIGHT_PWM);
    __tcu_disable_pwm_output(BACKLIGHT_PWM);

    __tcu_init_pwm_output_low(BACKLIGHT_PWM);
    __tcu_select_rtcclk(BACKLIGHT_PWM);
    __tcu_select_clk_div1(BACKLIGHT_PWM);

    __tcu_mask_half_match_irq(BACKLIGHT_PWM);
    __tcu_mask_full_match_irq(BACKLIGHT_PWM);
    
    old_val = -1;

    set_backlight_on();

    return true;
}

bool backlight_enabled(void)
{
    return old_val > -1 ? true : false;
}

void _backlight_on(void)
{
    set_backlight_on();
}

void _backlight_off(void)
{
    set_backlight_off();
}
#endif /* !SW_PWM */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void _backlight_set_brightness(int brightness)
{
    set_backlight(brightness);
}
#endif

#ifdef HAVE_LCD_SLEEP
/* Turn off LED supply */
void _backlight_lcd_sleep(void)
{
    set_backlight_off();
}
#endif
