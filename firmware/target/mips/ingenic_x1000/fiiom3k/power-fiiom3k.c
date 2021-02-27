/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "power.h"
#include "adc.h"
#include "system.h"
#include "kernel.h"
#include "axp173.h"

/* Copied from the FiiO Linux port -- remove once AXP driver works */
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3470
};

/* the OF shuts down at this voltage */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3400, 3639, 3697, 3723, 3757, 3786, 3836, 3906, 3980, 4050, 4159 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short const percent_to_volt_charge[11] =
{
      3485, 3780, 3836, 3857, 3890, 3930, 3986, 4062, 4158, 4185, 4196
};

void power_init(void)
{
    /* Does all the work */
    axp173_init();
}

void power_off(void)
{
    /* Set the shutdown bit */
    axp173_reg_set(0x32, 1 << 7);
    while(1);
}

bool charging_state(void)
{
    int r = axp173_reg_read(0x00);
    if(r < 0)
        return false;

    if(r & 0x04)
        return true;
    else
        return false;
}

unsigned int power_input_status(void)
{
    unsigned int state = 0;

    if(charging_state())
        state |= POWER_INPUT_MAIN_CHARGER;

    return state;
}

int _battery_voltage(void)
{
    return adc_read(ADC_BATTERY_VOLTAGE);
}
