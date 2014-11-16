/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#include <stdio.h>

#include "config.h"
#include "debug.h"
#include "panic.h"

#include "debug-ibasso.h"


/* See powermgmt.h. */


/*
    Values for stock PISEN battery.
    TODO: Needs optimization
*/
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3400
};


const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3350
};


/*
    Voltages (millivolt) of 0%, 10%, ... 100% when charging is disabled.
    Based on battery bench with stock Samsung battery.
*/
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3300, 3651, 3712, 3754, 3787, 3837, 3899, 3998, 4083, 4178, 4300 }
};


/* Voltages (millivolt) of 0%, 10%, ... 100% when charging is enabled. */
const unsigned short percent_to_volt_charge[11] =
{
    3300, 3651, 3712, 3754, 3787, 3837, 3899, 3998, 4083, 4178, 4300
};


int _battery_voltage( void )
{
    //TRACE;

    FILE *f = fopen( "/sys/class/power_supply/battery/voltage_now", "r" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open /sys/class/power_supply/battery/voltage_now for reading.", __func__ );
        panicf( "ERROR %s: Can not open /sys/class/power_supply/battery/voltage_now for reading.", __func__ );
        return 0;
    }

    int val;
    if( fscanf( f, "%d", &val ) == EOF )
    {
        DEBUGF( "ERROR %s: Read failed for /sys/class/power_supply/battery/voltage_now.", __func__ );
        val = 0;
    }

    fclose( f );

    return( val /  1000 );
}
