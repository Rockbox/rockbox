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


#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>

#include "config.h"
#include "debug.h"
#include "panic.h"
#include "power.h"

#include "button-ibasso.h"
#include "debug-ibasso.h"
#include "pcm-ibasso.h"


unsigned int power_input_status( void )
{
    //TRACE;

    FILE *f = fopen( "/sys/class/power_supply/ac/present", "r" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open /sys/class/power_supply/ac/present for reading.", __func__ );
        panicf( "ERROR %s: Can not open /sys/class/power_supply/ac/present for reading.", __func__ );
        return POWER_INPUT_NONE;
    }

    int val = 0;
    if( fscanf( f, "%d", &val ) == EOF )
    {
        DEBUGF( "ERROR %s: Read failed for /sys/class/power_supply/ac/present.", __func__ );
        val = 0;
    }

    fclose( f );

    return val ? POWER_INPUT_MAIN_CHARGER : POWER_INPUT_NONE;
}


void power_off( void )
{
    TRACE;

    button_close_device();

    reboot( RB_POWER_OFF );
}


/* Returns true, if battery is charging, false else. */
bool charging_state( void )
{
    //TRACE;

    FILE *f = fopen( "/sys/class/power_supply/battery/status", "r" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open /sys/class/power_supply/battery/status for reading.", __func__ );
        panicf( "ERROR %s: Can not open /sys/class/power_supply/battery/status for reading.", __func__ );
        return false;
    }

    /* Full, Charging, Discharging */
    char state[9] = { '\0' };

    if( fgets( state, 9, f ) == NULL )
    {
        DEBUGF( "ERROR %s: Read failed for /sys/class/power_supply/battery/status.", __func__ );
        state[0] = '\0';
    }

    fclose( f );

    return( strcmp( state, "Charging" ) == 0 );;
}
