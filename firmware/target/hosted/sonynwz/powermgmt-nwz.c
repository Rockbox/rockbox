/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include "powermgmt.h"
#include "power.h"
#include "power-nwz.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3470
};

/* the OF shuts down at this voltage */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3450
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3450, 3698, 3746, 3781, 3792, 3827, 3882, 3934, 3994, 4060, 4180 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short const percent_to_volt_charge[11] =
{
      3450, 3670, 3721, 3751, 3782, 3821, 3876, 3941, 4034, 4125, 4200
};

unsigned int power_input_status(void)
{
    unsigned pwr = 0;
    int sts = nwz_power_get_status();
    if(sts & NWZ_POWER_STATUS_VBUS_DET)
        pwr |= POWER_INPUT_USB_CHARGER;
    if(sts & NWZ_POWER_STATUS_AC_DET)
        pwr |= POWER_INPUT_MAIN_CHARGER;
    return pwr;
}

int _battery_voltage(void)
{
    /* the raw voltage is unstable on some devices, so use the average provided
     * by the driver */
    return nwz_power_get_battery_voltage();
}

bool charging_state(void)
{
    return (nwz_power_get_status() & NWZ_POWER_STATUS_CHARGE_STATUS) ==
        NWZ_POWER_STATUS_CHARGE_STATUS_CHARGING;
}
