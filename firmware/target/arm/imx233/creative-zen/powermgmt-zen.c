/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "powermgmt.h"
#include "power-imx233.h"
#include "led-imx233.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    0
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    0
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* Sansa Fuze+ Li Ion 600mAH figured from discharge curve */
    { 3100, 3650, 3720, 3750, 3780, 3820, 3880, 4000, 4040, 4125, 4230 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* Sansa Fuze+ Li Ion 600mAH figured from charge curve */
    3480, 3790, 3845, 3880, 3900, 3935, 4005, 4070, 4150, 4250, 4335
};

void charging_algorithm_init_hook(void)
{
#ifdef CREATIVE_ZEN
    /* The ZEN only has a blue LED, power it at 20% to get ~50% lightness */
    imx233_led_set_pwm(0, 0, 1000 /* Hz */, 20 /* % */);
#endif
}

void charging_algorithm_step_hook(void)
{
#if (defined(CREATIVE_ZENMOZAIC) || defined(CREATIVE_ZENXFI))
    /* The ZEN Mozaic and X-Fi have a red-green led: use it to show battery
     * level: green is 100%, red is 0%. */
    int idx = (battery_level() + 5) / 10;
    /* The table was obtained by applying gamma correction (x->x^3) in the range
     * 0.3 to 0.7, to obtain a change in hue but not in overall brightness */
    static int led_tbl[11] = {3, 4, 5, 7, 10, 13, 16, 20, 24, 29, 34};
    /* On those targets, channel 0 is red and 1 is green. See note below */
    int freq = 1000; /* Hz */
    imx233_led_set_pwm(0, 0, freq, led_tbl[10 - idx]);
    imx233_led_set_pwm(0, 1, freq, led_tbl[idx]);
#endif
}

void charging_algorithm_close_hook(void)
{
}
