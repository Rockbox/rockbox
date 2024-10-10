/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021 by Solomon Peachy
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
#include "power.h"
#include "panic.h"
#include "sysfs.h"

#include "tick.h"
#ifdef FIIO_M3K_LINUX
#include "usb.h"
#endif

#ifdef BATTERY_DEV_NAME
#define BATTERY_STATUS_PATH "/sys/class/power_supply/" BATTERY_DEV_NAME "/status"
#endif

#define POWER_STATUS_PATH "/sys/class/power_supply/" POWER_DEV_NAME "/online"

#ifdef BATTERY_DEV_NAME
/* We get called multiple times per tick, let's cut that back! */
static long last_tick = 0;
static bool last_power = false;

bool charging_state(void)
{
    if ((current_tick - last_tick) > HZ/2 ) {
        char buf[12] = {0};
        sysfs_get_string(BATTERY_STATUS_PATH, buf, sizeof(buf));

        last_tick = current_tick;
        last_power = (strncmp(buf, "Charging", 8) == 0);
    }
    return last_power;
}
#endif

unsigned int power_input_status(void)
{
    int present = 0;
    sysfs_get_int(POWER_STATUS_PATH, &present);

#ifdef FIIO_M3K_LINUX
    usb_enable(present ? true : false);
#endif

    return present ? POWER_INPUT_USB_CHARGER : POWER_INPUT_NONE;
}
