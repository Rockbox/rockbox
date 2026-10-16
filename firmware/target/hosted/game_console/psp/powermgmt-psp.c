/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Mauricio G.
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

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "config.h"
#include "kernel.h"
#include "powermgmt.h"
#include "power.h"
#include "adc.h"
#include "system.h"
#include "debug.h"

#include <psppower.h>

/*#define LOGF_ENABLE*/
#include "logf.h"

/* FIXME: what level should disksafe be?*/
unsigned short battery_level_disksafe = 0;

unsigned short battery_level_shutoff = 0;

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
{
      3450, 3670, 3721, 3751, 3782, 3821, 3876, 3941, 4034, 4125, 4200
};

enum
{
    BATT_CHARGING,
    BATT_NOT_CHARGING,
    CHARGER_CONNECTED,
    CHARGER_NOT_CONNECTED,
};

static int power_status = CHARGER_NOT_CONNECTED;
static int charging_status = BATT_NOT_CHARGING;

static void read_charger(void)
{
    int result = scePowerIsBatteryCharging();
    if (result < 0) {
        logf("warning, scePowerIsBatteryCharging returned 0x%x\n", result);
    }
    charging_status = scePowerGetBatteryChargingStatus() ? BATT_CHARGING : BATT_NOT_CHARGING;

    result = scePowerIsPowerOnline();
    if (result < 0) {
        logf("warning, scePowerIsPowerOnline returned 0x%x\n", result);
    }
    power_status = result == 1 ? CHARGER_CONNECTED : CHARGER_NOT_CONNECTED;
}

unsigned int power_input_status(void)
{
    read_charger();

    if (power_status == CHARGER_CONNECTED)
        return POWER_INPUT_MAIN_CHARGER;
    else
        return POWER_INPUT_NONE;
}

/* Returns battery voltage from MAX17040 VCELL ADC [millivolts steps],
 * adc returns voltage in 1.25mV steps */
/*
 * TODO this would be interesting to be mixed with battery percentage, for information
 * and completition purpouses
 */
int _battery_level(void)
{
    return scePowerGetBatteryLifePercent();
}

bool charging_state(void)
{
    return (power_status == CHARGER_CONNECTED && charging_status == BATT_CHARGING);
}

