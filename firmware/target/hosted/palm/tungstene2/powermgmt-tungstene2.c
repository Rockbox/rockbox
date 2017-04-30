/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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
#include "kernel.h"
#include "powermgmt.h"
#include "power.h"
#include "file.h"
#include "adc.h"
#include "stdbool.h"

#include "sysfs-unix.h"

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
    { 3450, 3502, 3550, 3587, 3623, 3669, 3742, 3836, 3926, 4026, 4200 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short const percent_to_volt_charge[11] =
{
      3450, 3670, 3721, 3751, 3782, 3821, 3876, 3941, 4034, 4125, 4200
};

static void read_charger(void)
{
    
}

unsigned int power_input_status(void)
{
	unsigned int result = POWER_INPUT_NONE;
	int charger_online = 0;

	sysfs_get_int(SYSFS_POWER_AC, &charger_online);
	if (charger_online)
	{
		result = POWER_INPUT_MAIN_CHARGER;
	}

    return result;
}

#endif /* CONFIG_CHARGING */


/* Returns battery voltage from ADC [millivolts],
 * adc returns voltage in 5mV steps */
int _battery_voltage(void)
{
    int voltage = 0;

    sysfs_get_int(SYSFS_POWER_BATTERY_VOLTAGE, &voltage);

    return voltage;
}

bool charging_state(void)
{
    return 0;
}
