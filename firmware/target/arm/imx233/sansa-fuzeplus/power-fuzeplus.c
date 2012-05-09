/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

#include "system.h"
#include "power.h"
#include "tuner.h"
#include "fmradio_i2c.h"
#include "pinctrl-imx233.h"
#include "power-imx233.h"

static bool tuner_enable = false;

bool tuner_power(bool enable)
{
    if(enable != tuner_enable)
    {
        /* CE is B029 (active high) */
        imx233_pinctrl_acquire_pin(0, 29, "tuner power");
        imx233_set_pin_function(0, 29, PINCTRL_FUNCTION_GPIO);
        imx233_set_pin_drive_strength(0, 29, PINCTRL_DRIVE_4mA);
        imx233_enable_gpio_output(0, 29, enable);
        imx233_set_gpio_output(0, 29, enable);
        tuner_enable = enable;
        /* give time to power up */
        udelay(5);
        //imx233_power_set_dcdc_freq(enable, HW_POWER_MISC__FREQSEL__24MHz);
    }
    return tuner_enable;
}

bool tuner_powered(void)
{
    return tuner_enable;
}
