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

/* PWM counter (PWM INITL = 0):
 * [0, PWM half rate[ -> backlight off
 * [PWM half rate, PWM full rate[ -> backlight on
 * [PWM full rate, PWM full rate] -> counter reset to 0
 */
static int old_val;
static void set_backlight(int val)
{
    if(val == old_val)
        return;

    /* The pulse repetition frequency should be greater than 100Hz so
       the flickering is not perceptible to the human eye but
       not greater than about 1kHz. */

    /* Clock = 32 768 Hz */
    /* 1 <= val <= 30 */
    int cycle = (MAX_BRIGHTNESS_SETTING - val + 1);

    __tcu_disable_pwm_output(BACKLIGHT_PWM);
    __tcu_stop_counter(BACKLIGHT_PWM);

    __tcu_set_count(BACKLIGHT_PWM, 0);
    __tcu_set_half_data(BACKLIGHT_PWM, cycle-1);
    __tcu_set_full_data(BACKLIGHT_PWM, cycle);

    __tcu_start_counter(BACKLIGHT_PWM);
    __tcu_enable_pwm_output(BACKLIGHT_PWM);

    old_val = val;
}

static void set_backlight_on(void)
{
    set_backlight(old_val);

    __tcu_enable_pwm_output(BACKLIGHT_PWM);
    __tcu_start_counter(BACKLIGHT_PWM);
}

static void set_backlight_off(void)
{
    __tcu_stop_counter(BACKLIGHT_PWM);
    __tcu_disable_pwm_output(BACKLIGHT_PWM);
}

bool _backlight_init(void)
{
    __gpio_as_pwm(BACKLIGHT_PWM);
    __tcu_start_timer_clock(BACKLIGHT_PWM);

    __tcu_stop_counter(BACKLIGHT_PWM);
    __tcu_init_pwm_output_low(BACKLIGHT_PWM);
    __tcu_set_pwm_output_shutdown_graceful(BACKLIGHT_PWM);
    __tcu_enable_pwm_output(BACKLIGHT_PWM);

    __tcu_select_rtcclk(BACKLIGHT_PWM); /* 32.768 kHz */
    __tcu_select_clk_div1(BACKLIGHT_PWM);

    __tcu_mask_half_match_irq(BACKLIGHT_PWM);
    __tcu_mask_full_match_irq(BACKLIGHT_PWM);

    old_val = MAX_BRIGHTNESS_SETTING;
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

#ifdef HAVE_LCD_SLEEP
/* Turn off LED supply */
void _backlight_lcd_sleep(void)
{
    set_backlight_off();
}
#endif
