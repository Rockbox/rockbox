/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright © 2009 Bertrik Sikken
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
#include "adc-target.h"
#include "powermgmt.h"

/* Battery voltage and discharge/charge curves for the Samsung YP-CP3.
 *
 * The original firmware (v2.10) measures the battery against the LSADC's
 * reference channel: mV = 637500 * ADC_BATTERY / (330 * ADC_VREF), each
 * channel averaged over a few samples. It refuses to boot under 3280 mV.
 *
 * TODO: the curves are the rk27xx generic board's, not measured here. */

unsigned short battery_level_disksafe = 3400; /* TODO: this is just an initial guess */

unsigned short battery_level_shutoff = 3300; /* TODO: this is just an initial guess */

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
    /* TODO: simple uncalibrated curve, linear except for first 10% */
    3300, 3600, 3665, 3730, 3795, 3860, 3925, 3990, 4055, 4120, 4185
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
    /* TODO: simple uncalibrated curve, linear except for first 10% */
    { 3300, 3600, 3665, 3730, 3795, 3860, 3925, 3990, 4055, 4120, 4185 };

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    unsigned int vref = adc_read(ADC_VREF);
    int mv = 0;

    if (vref != 0)
    {
        mv = 637500 * adc_read(ADC_BATTERY) / (330 * vref);
    }

    return mv;
}

