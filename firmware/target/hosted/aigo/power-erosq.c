/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 by Marcin Bukat
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
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "system.h"
#include "power-erosq.h"
#include "power.h"
#include "panic.h"
#include "sysfs.h"

const char * const sysfs_bat_voltage =
    "/sys/class/power_supply/battery/voltage_now";

const char * const sysfs_bat_capacity =
    "/sys/class/power_supply/battery/capacity";

const char * const sysfs_bat_status =
    "/sys/class/power_supply/battery/status";

const char * const sysfs_pow_supply =
    "/sys/class/power_supply/usb/present";

unsigned int erosq_power_input_status(void)
{
    int present = 0;
    sysfs_get_int(sysfs_pow_supply, &present);

    return present ? POWER_INPUT_USB_CHARGER : POWER_INPUT_NONE;
}

bool erosq_power_charging_status(void)
{
    char buf[12] = {0};
    sysfs_get_string(sysfs_bat_status, buf, sizeof(buf));

    return (strncmp(buf, "Charging", 8) == 0);
}

unsigned int erosq_power_get_battery_voltage(void)
{
    int battery_voltage;
    sysfs_get_int(sysfs_bat_voltage, &battery_voltage);

    return battery_voltage/1000;
}

unsigned int erosq_power_get_battery_capacity(void)
{
    int battery_capacity;
    sysfs_get_int(sysfs_bat_capacity, &battery_capacity);

    return battery_capacity;
}
