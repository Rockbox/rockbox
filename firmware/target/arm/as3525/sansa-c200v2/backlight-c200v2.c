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

int buttonlight_is_on = 0;
int backlight_is_on = 0;
static int backlight_level = 0;

/* logarithmic lookup table for brightness s*/
static const int brightness_table[MAX_BRIGHTNESS_SETTING+1] = {
    0, 21, 47, 78, 118, 165, 224, 296, 386, 495, 630, 796, 1000
};

static void _ll_backlight_on(void)
{
    if (c200v2_variant == 0) {
        GPIOA_PIN(5) = 1<<5;
    } else {
        GPIOA_PIN(7) = 1<<7;
    }
}

static void _ll_backlight_off(void)
{
    if (c200v2_variant == 0) {
        GPIOA_PIN(5) = 0;
    } else {
        GPIOA_PIN(7) = 0;
    }
}

static void _ll_buttonlight_on(void)
{
    if (c200v2_variant == 1) {
        /* Main buttonlight is on A5 */
        GPIOA_PIN(5) = 1<<5;
    } else {
        /* Needed for buttonlight and MicroSD to work at the same time */
        /* Turn ROD control on, as the OF does */
        GPIOD_DIR |= (1<<7);
        SD_MCI_POWER |= (1<<7);
        GPIOD_PIN(7) = (1<<7);
    }
}

static void _ll_buttonlight_off(void)
{
    if (c200v2_variant == 1) {
        /* Main buttonlight is on A5 */
        GPIOA_PIN(5) = 0;
    } else {
        /* Needed for buttonlight and MicroSD to work at the same time */
        /* Turn ROD control off, as the OF does */
        SD_MCI_POWER &= ~(1<<7);
        GPIOD_PIN(7) = 0;
        GPIOD_DIR &= ~(1<<7);
    }
}

void _backlight_pwm(int on)
{
    if (on) {
        if (backlight_is_on)
            _ll_backlight_on();

        if (buttonlight_is_on)
            _ll_buttonlight_on();
    } else {
        if (backlight_is_on)
            _ll_backlight_off();

        if (buttonlight_is_on)
            _ll_buttonlight_off();
    }
}

bool _backlight_init(void)
{
    GPIOA_DIR |= 1<<5;
    if (c200v2_variant == 1) {
        /* On this variant A7 is the backlight and
         * A5 is the buttonlight */
        GPIOA_DIR |= 1<<7;
    }
    return true;
}

void _backlight_set_brightness(int brightness)
{
    backlight_level = brightness_table[brightness];

    if (brightness > 0)
        _backlight_on();
    else
        _backlight_off();
}

static void _pwm_on(void)
{
    _set_timer2_pwm_ratio(backlight_level);
}

static void _pwm_off(void)
{
    if (buttonlight_is_on == 0 && backlight_is_on == 0)
        _set_timer2_pwm_ratio(0);
}

void _backlight_on(void)
{
    if (backlight_is_on == 1) {
        /* Update pwm ratio in case user changed the brightness */
        _pwm_on();
        return;
    }

#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    _ll_backlight_on();
    _pwm_on();
    backlight_is_on = 1;
}

void _backlight_off(void)
{
    if (backlight_is_on == 0)
        return;

    backlight_is_on = 0;
    _pwm_off();
    _ll_backlight_off();
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

void _buttonlight_on(void)
{
    if (buttonlight_is_on == 1)
        return;

    _ll_buttonlight_on();
    _pwm_on();
    buttonlight_is_on = 1;
}

void _buttonlight_off(void)
{
    if (buttonlight_is_on == 0)
        return;

    buttonlight_is_on = 0;
    _pwm_off();
    _ll_buttonlight_off();
}
