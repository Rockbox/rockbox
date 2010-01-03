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
#include "lcd.h"

/* PWM_CHN7 == GPIO(32*3 + 31) */
#define BACKLIGHT_GPIO  (32*3+31)
#define BACKLIGHT_PWM   7

/* PWM counter (PWM INITL = 0):
 * [0, PWM half rate[ -> backlight off
 * [PWM half rate, PWM full rate[ -> backlight on
 * [PWM full rate, PWM full rate] -> counter reset to 0
 */
static int old_val = MAX_BRIGHTNESS_SETTING;
static const unsigned char logtable[] = {255, 254, 253, 252, 250, 248, 245, 240, 233, 224, 211, 192, 165, 128, 75, 0};
static void set_backlight(int val)
{
    if(val == old_val)
        return;

    __tcu_disable_pwm_output(BACKLIGHT_PWM);
    __tcu_stop_counter(BACKLIGHT_PWM);

    __tcu_set_count(BACKLIGHT_PWM, 0);
    __tcu_set_half_data(BACKLIGHT_PWM, logtable[val - 1]);
    __tcu_set_full_data(BACKLIGHT_PWM, 256);

    __tcu_start_counter(BACKLIGHT_PWM);
    __tcu_enable_pwm_output(BACKLIGHT_PWM);

    old_val = val;
}

static void set_backlight_on(void)
{
    __tcu_start_counter(BACKLIGHT_PWM);
    __tcu_enable_pwm_output(BACKLIGHT_PWM);
}

static void set_backlight_off(void)
{
    __tcu_disable_pwm_output(BACKLIGHT_PWM);
    __tcu_stop_counter(BACKLIGHT_PWM);
}

bool _backlight_init(void)
{
    __gpio_as_pwm(BACKLIGHT_PWM);
    __tcu_start_timer_clock(BACKLIGHT_PWM);

    __tcu_stop_counter(BACKLIGHT_PWM);
    __tcu_init_pwm_output_low(BACKLIGHT_PWM);
    __tcu_set_pwm_output_shutdown_graceful(BACKLIGHT_PWM);
    __tcu_disable_pwm_output(BACKLIGHT_PWM);

    __tcu_select_extalclk(BACKLIGHT_PWM);  /* 12 MHz */
    __tcu_select_clk_div64(BACKLIGHT_PWM); /* 187.5 kHz */

    __tcu_mask_half_match_irq(BACKLIGHT_PWM);
    __tcu_mask_full_match_irq(BACKLIGHT_PWM);

    __tcu_set_count(BACKLIGHT_PWM, 0);
    __tcu_set_half_data(BACKLIGHT_PWM, 0);
    __tcu_set_full_data(BACKLIGHT_PWM, 256);

    return true;
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd */
#endif
    set_backlight_on();
}

void _backlight_off(void)
{
    set_backlight_off();
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off lcd */
#endif
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
    _backlight_off();
}
#endif
