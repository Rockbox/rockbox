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
#include "power-agptek.h"
#include "power.h"
#include "panic.h"
#include "sysfs.h"

const char * const sysfs_bat_voltage =
    "/sys/class/power_supply/battery/voltage_now";

const char * const sysfs_bat_status =
    "/sys/class/power_supply/battery/status";

unsigned int agptek_power_get_status(void)
{
    char buf[12] = {0};
    sysfs_get_string(sysfs_bat_status, buf, sizeof(buf));

    if (strncmp(buf, "Charging", 8) == 0)
    {
        return POWER_INPUT_USB_CHARGER;
    }
    else
    {
        return POWER_INPUT_NONE;
    }
}

unsigned int agptek_power_get_battery_voltage(void)
{
    int battery_voltage;
    sysfs_get_int(sysfs_bat_voltage, &battery_voltage);

    return battery_voltage/1000;
}
