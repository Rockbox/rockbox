/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "adc.h"
#include "powermgmt.h"

/* FIXME: calibrate values */
unsigned short battery_level_disksafe = 3380;
unsigned short battery_level_shutoff = 3300;

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
    /* FIXME: calibrate values. Table is "inherited" from iPod-PCF / H100 */
    3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
{
    /* FIXME: calibrate values. Table is "inherited" from iPod-PCF / H100 */
    3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 4200
/* Nominal full-scale battery voltage in millivolts: PWRADC is 2.8V and
 * BATT_LEV uses R304=100k / R305=200k, so VBAT = VADC * 3/2.
 * The divider's switching transistor and component tolerances still need
 * checking against a physical voltage measurement on the player. */

/* Return millivolts from the TCC7801's interrupt-updated ADC channel cache. */
int _battery_voltage(void)
{
    return (adc_read(ADC_BATTERY) * BATTERY_SCALE_FACTOR) >> 10;
}
