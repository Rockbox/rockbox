/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#include "adc.h"
#include "powermgmt.h"
#include "kernel.h"

/* Use fake linear scale as AVR does the voltage to percentage conversion */

static unsigned int current_battery_level = 100;

/* This specifies the battery level that writes are still safe */
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    5
};

/* Below this the player cannot be considered to operate reliably */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    4
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
};

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    return current_battery_level;
}

void set_battery_level(unsigned int level)
{
    current_battery_level = level;
}

