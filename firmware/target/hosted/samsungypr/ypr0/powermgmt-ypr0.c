/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "radio-ypr.h"
#include "ascodec.h"
#include "stdbool.h"

enum
{
    BATT_CHARGING,
    BATT_NOT_CHARGING,
    CHARGER_CONNECTED,
    CHARGER_NOT_CONNECTED,
};

static bool first_readout = true;
static int power_status = CHARGER_NOT_CONNECTED;
static int charging_status = BATT_NOT_CHARGING;

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
    charging_status = ascodec_endofch() ? BATT_NOT_CHARGING : BATT_CHARGING;
    power_status = ascodec_chg_status() ? CHARGER_CONNECTED : CHARGER_NOT_CONNECTED;
    /* Sync the filter due to new charging state */
    reset_battery_filter(_battery_voltage());
}

unsigned int power_input_status(void)
{
    if (first_readout)
    {
        /* 350mA, 4.20V */
        ascodec_write_pmu(AS3543_CHARGER, 0x1, 0x5C);
        /* Enable interrupt for charging detection */
        ascodec_write(AS3514_IRQ_ENRD0, CHG_CHANGED);
        read_charger();
        first_readout = false;
    }

    if (ascodec_read(AS3514_IRQ_ENRD0) & CHG_CHANGED)
    {
        /* Something has changed... */
        read_charger();
    }

    if (power_status == CHARGER_CONNECTED)
        return POWER_INPUT_MAIN_CHARGER;
    else
        return POWER_INPUT_NONE;
}

#endif /* CONFIG_CHARGING */


/* Returns battery voltage from ADC [millivolts],
 * adc returns voltage in 5mV steps */
int _battery_voltage(void)
{
    return adc_read(3) * 5;
}

bool charging_state(void)
{
    return (power_status == CHARGER_CONNECTED && charging_status == BATT_CHARGING);
}

#if CONFIG_TUNER
static bool tuner_on = false;

bool tuner_power(bool status)
{
    if (status != tuner_on)
    {
        tuner_on = status;
        status = !status;
        if (tuner_on) {
            radiodev_open();
        }
        else {
            radiodev_close();
        }
    }

    return status;
}

bool tuner_powered(void)
{
    return tuner_on;
}
#endif /* #if CONFIG_TUNER */