/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: powermgmt-nano2g.c 28159 2010-09-24 22:42:06Z Buschel $
 *
 * Copyright © 2008 Rafaël Carré
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
#include "powermgmt.h"
#include "pmu-target.h"
#include "power.h"
#include "audiohw.h"
#include "adc-target.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3500
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3500, 3670, 3720, 3750, 3770, 3800, 3860, 3920, 3980, 4070, 4170 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3700, 3820, 3900, 3950, 3990, 4030, 4070, 4120, 4170, 4190, 4200
};
#endif /* CONFIG_CHARGING */

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    return adc_read_battery_voltage();
}

#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool enable)
{
    if (enable)
    {
        /* Accessory voltage supply on */
        pmu_ldo_power_on(6);
    }
    else
    {
        /* Accessory voltage supply off */
        pmu_ldo_power_off(6);
    }
}
#endif

#ifdef HAVE_LINEOUT_POWEROFF
void lineout_set(bool enable)
{
    /* Call audio hardware driver implementation */
    audiohw_enable_lineout(enable);
}
#endif
