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
#include "tick.h"

const char * const sysfs_bat_voltage[2] = {
    "/sys/class/power_supply/battery/voltage_now",
    "/sys/class/power_supply/axp_battery/voltage_now",
};

const char * const sysfs_bat_capacity[2] = {
    "/sys/class/power_supply/battery/capacity",
    "/sys/class/power_supply/axp_battery/capacity",
};

const char * const sysfs_bat_status[2] = {
    "/sys/class/power_supply/battery/status",
    "/sys/class/power_supply/axp_battery/status",
};

static int hwver = 0;

unsigned int erosq_power_get_battery_voltage(void)
{
    int battery_voltage;
    int x = sysfs_get_int(sysfs_bat_voltage[hwver], &battery_voltage);

    if (!x) {
        hwver ^= 1;
        sysfs_get_int(sysfs_bat_voltage[hwver], &battery_voltage);
    }

    return battery_voltage/1000;
}

unsigned int erosq_power_get_battery_capacity(void)
{
    int battery_capacity;
    int x = sysfs_get_int(sysfs_bat_capacity[hwver], &battery_capacity);

    if (!x) {
        hwver ^= 1;
        sysfs_get_int(sysfs_bat_capacity[hwver], &battery_capacity);
    }

    return battery_capacity;
}

/* We get called multiple times per tick, let's cut that back! */
static long last_tick = 0;
static bool last_power = false;

bool charging_state(void)
{
    if ((current_tick - last_tick) > HZ/2 ) {
        char buf[12] = {0};
        int x = sysfs_get_string(sysfs_bat_status[hwver], buf, sizeof(buf));

        if (!x) {
            hwver ^= 1;
            sysfs_get_string(sysfs_bat_status[hwver], buf, sizeof(buf));
        }

        last_tick = current_tick;
        last_power = (strncmp(buf, "Charging", 8) == 0);
    }
    return last_power;
}
