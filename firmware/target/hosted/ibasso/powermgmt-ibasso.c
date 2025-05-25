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
#include <string.h>
#include "config.h"
#include "debug.h"
#include "panic.h"

#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


/* Based on batterymonitor with PISEN and Samsung SIII battery. */


unsigned short battery_level_disksafe = 3600;

unsigned short battery_level_shutoff = 3500;

/*
    Averages at percent of running time from five measuremnts with PISEN and Samsung SIII battery
    during normal usage.

    Mongo default values (?)
    < 3660 (0%), < 3730 (1% - 10%), < 3780 (11% - 20%), < 3830 (21% - 40%), < 3950 (41% - 60%),
    < 4080 (61% - 80%), > 4081 (81% - 100%)
*/
unsigned short percent_to_volt_discharge[11] =
{
    3522, 3660, 3720, 3752, 3784, 3827, 3896, 3978, 4072, 4168, 4255
};


/* Copied from percent_to_volt_discharge. */
unsigned short percent_to_volt_charge[11] =
{
    3500, 3544, 3578, 3623, 3660, 3773, 3782, 3853, 3980, 4130, 4360
};


static int _battery_present = -1;


int _battery_voltage(void)
{
    /*TRACE;*/

    if(   (_battery_present == -1)
       && (! sysfs_get_int(SYSFS_BATTERY_PRESENT, &_battery_present)))
    {
        /* This check is only done once at startup. */

        DEBUGF("ERROR %s: Can not get current battery availabilty.", __func__);
        _battery_present = 1;
    }

    int val;

    if(_battery_present == 1)
    {
        /* Battery is present. */

        /*
            /sys/class/power_supply/battery/voltage_now
            Voltage in microvolt.
        */
        if(! sysfs_get_int(SYSFS_BATTERY_VOLTAGE_NOW, &val))
        {
            DEBUGF("ERROR %s: Can not get current battery voltage.", __func__);
            return 0;
        }
    }
    else
    {
        /*
            No battery, so we have to be running solely from USB power.
            This will prevent Rockbox from forcing shutdown due to low power.
        */

        /*
            /sys/class/power_supply/usb/voltage_now
            Voltage in microvolt.
        */
        if(! sysfs_get_int(SYSFS_USB_POWER_VOLTAGE_NOW, &val))
        {
            DEBUGF("ERROR %s: Can not get current USB voltage.", __func__);
            return 0;
        }
    }

    return(val / 1000);
}

/* NOTE:  This is largely lifted from the generic hostedpower-linux.c
   but that uses a different sysfs api (expecting a path strning rather than
   an enumeration */
#include "tick.h"
#include "power.h"

static long last_tick = 0;
static bool last_power = false;
bool charging_state(void)
{
    if ((current_tick - last_tick) > HZ/2 ) {
        char buf[12] = {0};
        sysfs_get_string(SYSFS_BATTERY_STATUS, buf, sizeof(buf));

        last_tick = current_tick;
        last_power = (strncmp(buf, "Charging", 8) == 0);
    }
    return last_power;
}

unsigned int power_input_status(void)
{
    int present = 0;
    sysfs_get_int(SYSFS_USB_POWER_ONLINE, &present);

    return present ? POWER_INPUT_USB_CHARGER : POWER_INPUT_NONE;
}
