/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "tsc2100.h"
#include "kernel.h"

static unsigned short current_voltage = 3910;
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
    { 100, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1320 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    100, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1320,
};

void read_battery_inputs(void)
{
    short dummy1, dummy2;
    tsc2100_read_volt(&current_voltage, &dummy1, &dummy2);

    /* Set the TSC2100 back to read touches */
    tsc2100_set_mode(0x01);
}
    
/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    static unsigned last_tick = 0;

    if (TIME_BEFORE(last_tick+2*HZ, current_tick))
    {
        /* Set the TSC2100 to read voltages */
        tsc2100_set_mode(0x0B);
        last_tick = current_tick;
    }
        
    return current_voltage;
}

