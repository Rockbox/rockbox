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
#include <stdlib.h>
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


/* Returns true, if battery is charging, false else. */
bool charging_state( void )
{
    /* Full, Charging, Discharging */
    char state[9];

    /* true if charging. */
    bool charging = false;

    FILE *f = fopen( "/sys/class/power_supply/battery/status", "r" );
    if( f != NULL )
    {
        if( fgets( state, 9, f ) != NULL )
        {
            charging = ( strcmp( state, "Charging" ) == 0 );
        }
    }
    fclose( f );

    return charging;
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

/* Voltages (millivolt) of 0%, 10%, ... 100% when charging is enabled. */
const unsigned short percent_to_volt_charge[11] =
{
    3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4090, 4190
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


