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
#include "power.h"

#define BATT_MINMVOLT   3300      /* minimum millivolts of battery */
#define BATT_MAXMVOLT   4300      /* maximum millivolts of battery */
#define BATT_MAXRUNTIME (10 * 60) /* maximum runtime with full battery in
                                     minutes */

extern void send_battery_level_event(void);
extern int last_sent_battery_level;
extern int battery_percent;
static bool charging = false;

static unsigned int battery_millivolts = BATT_MAXMVOLT;

void powermgmt_init_target(void) {}

static void battery_status_update(void)
{
    static long last_tick = 0;

    if (TIME_AFTER(current_tick, (last_tick+HZ))) {
        last_tick = current_tick;

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
    }
}

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] = { 3200 };
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] = { 3200 };

/* make the simulated curve nicely linear */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{ { 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200, 4300 } };
const unsigned short percent_to_volt_charge[11] =
{ 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200, 4300  };


int _battery_voltage(void)
{
    battery_status_update();
    return battery_millivolts;
}

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    return charging ? POWER_INPUT_NONE : POWER_INPUT_MAIN;
}

bool charging_state(void)
{
    return charging;
}
#endif

#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool enable)
{
    (void)enable;
}
#endif

#ifdef HAVE_LINEOUT_POWEROFF
void lineout_set(bool enable)
{
    (void)enable;
}
#endif

#ifdef HAVE_REMOTE_LCD
bool remote_detect(void)
{
    return true;
}
#endif

#ifdef HAVE_BATTERY_SWITCH
unsigned int input_millivolts(void)
{
    if ((power_input_status() & POWER_INPUT_BATTERY) == 0) {
        /* Just return a safe value if battery isn't connected */
        return 4050;
    }
    return battery_voltage();;
}
#endif
