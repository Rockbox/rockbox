/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2017 by Amaury Pouly
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
#ifndef __LED_IMX233_H__
#define __LED_IMX233_H__

#include "system.h"

/** LED API
 *
 * The following assumes each single LED is controllable by one
 * or more channels. Each channel is either controlled by a GPIO (on/off) or a
 * PWM. Each channel also has a color, so it is possible to describe
 * a red-green LED for example. */

/* LED channel color */
enum imx233_led_color_t
{
    LED_RED,
    LED_GREEN,
    LED_BLUE,
};

/* LED channel */
struct imx233_led_chan_t
{
    bool has_pwm; /* GPIO or PWM */
    enum imx233_led_color_t color; /* color */
    int gpio_bank, gpio_pin; /* GPIO bank and pin (also valid for PWM) */
    int pwm_chan; /* for PWM: harware channel*/
};

/* LED */
struct imx233_led_t
{
    int nr_chan; /* number of channels */
    struct imx233_led_chan_t *chan; /* array of channels */
};

/* init: all LEDs are turned off on init */
void imx233_led_init(void);
/* get number of LEDs */
int imx233_led_get_count(void);
/* return an array of LEDs */
struct imx233_led_t *imx233_led_get_info(void);
/* set LED channel on/off, it also works for PWM channel by setting the PWM to
 * constantly off or constantly on */
void imx233_led_set(int led, int chan, bool on);
/* set LED PWM: control the frequency and the duty cycle percentage (0-100) */
void imx233_led_set_pwm(int led, int chan, int freq, int duty);

#endif /* __LED_IMX233_H__ */
