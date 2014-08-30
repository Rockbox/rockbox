/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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

#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "power.h"
#include "powermgmt.h"

unsigned int power_input_status(void)
{
    int val;
    FILE *f = fopen("/sys/class/power_supply/ac/present", "r");
    fscanf(f, "%d", &val);
    fclose(f);
    return val?POWER_INPUT_MAIN_CHARGER:POWER_INPUT_NONE;
}

/* Values for stock PISEN battery. TODO: Needs optimization */

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3380
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3100
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4090, 4190 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3540, 3860, 3930, 3980, 4000, 4020, 4040, 4080, 4130, 4180, 4220 /* LiPo */
};


/* Returns battery voltage from android measurement [millivolts] */
int _battery_voltage(void)
{
    int val;
    FILE *f = fopen("/sys/class/power_supply/battery/voltage_now", "r");
    fscanf(f, "%d", &val);
    fclose(f);
    return (val/1000);
}


