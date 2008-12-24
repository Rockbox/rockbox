/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Heikki Hannikainen, Uwe Freese
 * Revisions copyright (C) 2005 by Gerald Van Baren
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

#define BATT_MINMVOLT   2500      /* minimum millivolts of battery */
#define BATT_MAXMVOLT   4500      /* maximum millivolts of battery */
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
    static bool charging = false;
    time_t now;

    time(&now);

    if (last_change < now) {
        last_change = now;

        /* change the values: */
        if (charging) {
            if (battery_millivolts >= BATT_MAXMVOLT) {
                /* Pretend the charger was disconnected */
                charging = false;
                queue_broadcast(SYS_CHARGER_DISCONNECTED, 0);
                last_sent_battery_level = 100;
            }
        }
        else {
            if (battery_millivolts <= BATT_MINMVOLT) {
                /* Pretend the charger was connected */
                charging = true;
                queue_broadcast(SYS_CHARGER_CONNECTED, 0);
                last_sent_battery_level = 0;
            }
        }

        if (charging) {
            battery_millivolts += (BATT_MAXMVOLT - BATT_MINMVOLT) / 50;
        }
        else {
            battery_millivolts -= (BATT_MAXMVOLT - BATT_MINMVOLT) / 100;
        }

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

void set_poweroff_timeout(int timeout)
{
    (void)timeout;
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

#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool enable)
{
    (void)enable;
}
#endif

void reset_poweroff_timer(void)
{
}

void shutdown_hw(void)
{
}

void sys_poweroff(void)
{
}

void cancel_shutdown(void)
{
}
