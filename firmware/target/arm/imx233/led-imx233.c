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
#include "led-imx233.h"
#include "pwm-imx233.h"
#include "pinctrl-imx233.h"

/** Target specific tables */
#if defined(CREATIVE_ZENXFI) || defined(CREATIVE_ZENMOZAIC)
/* ZEN X-Fi/Mozaic have a Red-Green LED */
static struct imx233_led_chan_t zenxfi_led_chans[] =
{
    { true, LED_RED, 2, 2, 2 }, /* red channel on B20P2 (pwm 2) */
    { true, LED_GREEN, 2, 4, 4 }, /* green channel on B20P4 (pwm 4) */
};
static struct imx233_led_t leds[] = {{2, zenxfi_led_chans}};
#elif defined(CREATIVE_ZEN)
/* ZEN has a blue LED */
static struct imx233_led_chan_t zen_led_chans[] =
{
    { true, LED_BLUE, 2, 3, 3 }, /* blue channel on B20P3 (pwm 3) */
};
static struct imx233_led_t leds[] = {{1, zen_led_chans}};
#else
#define NO_LEDS
#endif

#ifndef NO_LEDS
int imx233_led_get_count(void)
{
    return sizeof(leds) / sizeof(leds[0]);
}

struct imx233_led_t *imx233_led_get_info(void)
{
    return leds;
}
#else
int imx233_led_get_count(void)
{
    return 0;
}

struct imx233_led_t * imx233_led_get_info(void)
{
    return NULL;
}
#endif

void imx233_led_init(void)
{
    struct imx233_led_t *leds = imx233_led_get_info();
    /* turn off all channels */
    for(int led = 0; led < imx233_led_get_count(); led++)
        for(int chan = 0; chan < leds[led].nr_chan; chan++)
            imx233_led_set(led, chan, false);
}

void imx233_led_set(int led, int chan, bool on)
{
    struct imx233_led_chan_t *c = &imx233_led_get_info()[led].chan[chan];
    /* if LED has a PWM, handle it using the PWM */
    if(c->has_pwm)
    {
        /* toogle at 1KHz */
        return imx233_led_set_pwm(led, chan, 1000, on ? 100 : 0);
    }
    /* make sure pin is configured as a GPIO */
    imx233_pinctrl_setup_vpin(
        VPIN_PACK(c->gpio_bank, c->gpio_pin, GPIO), "led",
        PINCTRL_DRIVE_4mA, false);
    imx233_pinctrl_enable_gpio(c->gpio_bank, c->gpio_pin, true);
    imx233_pinctrl_set_gpio(c->gpio_bank, c->gpio_pin, on);
}

void imx233_led_set_pwm(int led, int chan, int freq, int duty)
{
    struct imx233_led_chan_t *c = &imx233_led_get_info()[led].chan[chan];
    if(!c->has_pwm)
        panicf("led %d chan %d cannot do pwm", led, chan);
    imx233_pwm_setup_simple(c->pwm_chan, freq, duty);
    imx233_pwm_enable(c->pwm_chan, true);
}
