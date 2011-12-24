/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: powermgmt-sim.c 29543 2011-03-08 19:33:30Z thomasjfox $
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
#include "system.h"
#include <time.h>
#include "kernel.h"
#include "powermgmt.h"
#include "ascodec-target.h"
#include "stdio.h"

#if 0 /*still unused*/
/* The battery manufacturer's website shows discharge curves down to 3.0V,
   so 'dangerous' and 'shutoff' levels of 3.4V and 3.3V should be safe.
 */
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3550
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3450
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3300, 3692, 3740, 3772, 3798, 3828, 3876, 3943, 4013, 4094, 4194 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3417, 3802, 3856, 3888, 3905, 3931, 3973, 4025, 4084, 4161, 4219
};
#endif /* CONFIG_CHARGING */
#endif

#define BATT_MINMVOLT   3450      /* minimum millivolts of battery */
#define BATT_MAXMVOLT   4150      /* maximum millivolts of battery */
#define BATT_MAXRUNTIME (10 * 60) /* maximum runtime with full battery in
                                     minutes */

extern void send_battery_level_event(void);
extern int last_sent_battery_level;
extern int battery_percent;

static unsigned int battery_millivolts = BATT_MAXMVOLT;
/* estimated remaining time in minutes */
static int powermgmt_est_runningtime_min = BATT_MAXRUNTIME;

static void battery_status_update(void)
{
    static time_t last_change = 0;
    time_t now;

    time(&now);

    if (last_change < now) {
        last_change = now;

        battery_percent = 100 * (battery_millivolts - BATT_MINMVOLT) /
                            (BATT_MAXMVOLT - BATT_MINMVOLT);

        powermgmt_est_runningtime_min =
            battery_percent * BATT_MAXRUNTIME / 100;
    }

    send_battery_level_event();
}

void battery_read_info(int *voltage, int *level)
{
    battery_status_update();

    if (voltage)
        *voltage = battery_millivolts;

    if (level)
        *level = battery_percent;
}

unsigned int battery_voltage(void)
{
    battery_status_update();
    return battery_millivolts;
}

int battery_level(void)
{
    battery_status_update();
    return battery_percent;
}

int battery_time(void)
{
    battery_status_update();
    return powermgmt_est_runningtime_min;
}

bool battery_level_safe(void)
{
    return battery_level() >= 10;
}

void set_battery_capacity(int capacity)
{
  (void)capacity;
}

#if BATTERY_TYPES_COUNT > 1
void set_battery_type(int type)
{
    (void)type;
}
#endif
