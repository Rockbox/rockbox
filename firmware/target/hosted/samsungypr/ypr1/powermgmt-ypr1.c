/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "kernel.h"
#include "powermgmt.h"
#include "power.h"
#include "adc.h"
#include "radio-ypr.h"
#include "pmu-ypr1.h"
#include "ioctl-ypr1.h"
#include "system.h"

#define MAX17040_VCELL          0x02
#define MAX17040_SOC            0x04
#define MAX17040_MODE           0x06
#define MAX17040_VERSION        0x08
#define MAX17040_RCOMP          0x0C
#define MAX17040_COMMAND        0xFE

static int max17040_dev = -1;

void max17040_init(void)
{
    max17040_dev = open("/dev/r1Batt", O_RDONLY);
    if (max17040_dev < 0)
        printf("/dev/r1Batt open error!");
}

void max17040_close(void)
{
    if (max17040_dev >= 0)
        close(max17040_dev);
}

#if (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE) == VOLTAGE_MEASURE
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
#endif

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short const percent_to_volt_charge[11] =
{
      3450, 3670, 3721, 3751, 3782, 3821, 3876, 3941, 4034, 4125, 4200
};

unsigned int power_input_status(void)
{
    unsigned status = POWER_INPUT_NONE;
    if (pmu_ioctl(MAX8819_IOCTL_IS_EXT_PWR, NULL) > 0)
        status = POWER_INPUT_MAIN_CHARGER;
    return status;
}

#endif /* CONFIG_CHARGING */

/* Returns battery voltage from MAX17040 VCELL ADC [millivolts steps],
 * adc returns voltage in 1.25mV steps */
/*
 * TODO this would be interesting to be mixed with battery percentage, for information
 * and completition purpouses
 */
#if (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE) == VOLTAGE_MEASURE
int _battery_voltage(void)
{
    int level = 4000;
    max17040_request ret = { .addr = 2, .reg1 = 0, .reg2 = 0 };
    if (ioctl(max17040_dev, MAX17040_READ_REG, &ret) >= 0)
    {
        int step = (ret.reg1 << 4) | (ret.reg2 >> 4);
        level = step + (step >> 2);
    }
    return level;
}
#elif (CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE) == PERCENTAGE_MEASURE
int _battery_level(void)
{
    int level = 100;
    max17040_request ret = { .addr = 4, .reg1 = 0, .reg2 = 0 };
    if (ioctl(max17040_dev, MAX17040_READ_REG, &ret) >= 0)
        level = MIN(ret.reg1, 100);
    return level;
}
#endif

bool charging_state(void)
{
    int ret = pmu_ioctl(MAX8819_IOCTL_GET_CHG_STATUS, NULL);
    if (ret == PMU_FULLY_CHARGED)
        return true;
    return false;
}

#if CONFIG_TUNER
static bool tuner_on = false;

bool tuner_power(bool status)
{
    if (status != tuner_on)
    {
        tuner_on = status;
        status = !status;
        if (tuner_on)
            radiodev_open();
        else
            radiodev_close();
    }

    return status;
}

bool tuner_powered(void)
{
    return tuner_on;
}
#endif /* #if CONFIG_TUNER */
