/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Lorenzo Miori
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
#include "system.h"
#include "lcd.h"
#include "backlight.h"
#include "backlight-target.h"
#include "pinctrl-imx233.h"

/**
 * AAT3151 Backlight Controller
 */

/* Timings */
#define TIME_OFF                500
#define TIME_LOW                50
#define TIME_HI                 50
#define TIME_LAT                500

/* Number of raising edges to select the particular register */
#define D1_D4_CURRENT_E           17
#define D1_D3_CURRENT_E           18
#define D4_CURRENT_E              19
#define MAX_CURRENT_E             20
#define LOW_CURRENT_E             21

/* The actual register address / number */
#define D1_D4_CURRENT           1
#define D1_D3_CURRENT           2
#define D4_CURRENT              3
#define MAX_CURRENT             4
#define LOW_CURRENT             5

/* Valid values for LOW_CURRENT register */
#define MAX_CURRENT_20                  1
#define MAX_CURRENT_30                  2
#define MAX_CURRENT_15                  3
#define MAX_CURRENT_LOW_CURRENT         4

static int current_level = -1;

static void create_raising_edges(int num)
{
    while (num--)
    {
        /* Setting a register takes a sufficient small amount of time,
         * in the order of 50 ns. Thus the necessary 2 delays TIME_LOW/TIME_HI
         * are not strictly necessary */
        imx233_pinctrl_set_gpio(3, 13, false);
        imx233_pinctrl_set_gpio(3, 13, true);
    }
}

static void aat3151_write(int addr, int data)
{
    create_raising_edges(16 + addr);
    udelay(TIME_LAT);
    create_raising_edges(data);
    udelay(TIME_LAT);
}

void backlight_hw_brightness(int level)
{
    /* Don't try to reset backlight if not necessary
     *  Moreover this helps to avoid flickering when
     *  being in some screens like USB mode and
     *  pressing some keys / touchpad...
     */
    if (current_level == level) return;

    /* Check for limits and adjust in case */
    level = MIN(MAX_BRIGHTNESS_SETTING, MAX(0, level));

    if (level == 0)
    {
        /* Set pin low for a sufficient time, puts the device into low-power consumption state
         * In other words backlight goes off
         */
        imx233_pinctrl_set_gpio(3, 13, false);
        udelay(TIME_OFF);
    }
    else
    {
        if (level > 3) {
            /* This enables 16 levels of backlight */
            aat3151_write(MAX_CURRENT, MAX_CURRENT_15);
            /* Set the value according Table 1 in datasheet
             * For MAX_CURRENT_15, the scale is from 0 mA to 15 mA in 16 steps
             */
            aat3151_write(D1_D3_CURRENT, 19 - level);
        }
        else {
            /* This enables other 4 levels of backlight */
            aat3151_write(MAX_CURRENT, MAX_CURRENT_LOW_CURRENT);
            /* Set the value according Table 1 in datasheet
             * For LOW_CURRENT, there is no "real" scale. We have scattered values.
             * We are interested in the last 3 -> 0.5 mA; 1 mA; 2 mA
             */
            aat3151_write(LOW_CURRENT, 13 + level);
        }
    }
    current_level = level;
}

bool backlight_hw_init(void)
{
    imx233_pinctrl_acquire(3, 13, "backlight");
    imx233_pinctrl_set_function(3, 13, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_set_drive(3, 13, PINCTRL_DRIVE_4mA);
    imx233_pinctrl_enable_gpio(3, 13, true);
    imx233_pinctrl_set_gpio(3, 13, false);
    return true;
}

void backlight_hw_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    /* restore the previous backlight level */
    backlight_hw_brightness(backlight_brightness);
}

void backlight_hw_off(void)
{
    /* there is no real on/off but we can set to 0 brightness */
    backlight_hw_brightness(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}
