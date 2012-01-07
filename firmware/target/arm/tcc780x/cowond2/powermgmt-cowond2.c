/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "kernel.h"
#include "pmu-target.h"
#include "pcf50606.h"
#include "pcf50635.h"

unsigned short current_voltage = 3910;

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3380
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* Standard D2 internal battery */
    { 3370, 3690, 3750, 3775, 3790, 3820, 3880, 3940, 3980, 4090, 4170 }

    /* TODO: DIY replacements eg. Nokia BP-4L ? */
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* FIXME: voltages seem to be offset during charging (eg. 4500+) */
    3370, 3690, 3750, 3775, 3790, 3820, 3880, 3940, 3980, 4090, 4170
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 6000
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    static unsigned last_tick = 0;

    if (TIME_BEFORE(last_tick+HZ, current_tick))
    {
        short adc_val;
        
        if (get_pmu_type() == PCF50606)
            pcf50606_read_adc(PCF5060X_ADC_BATVOLT_RES, &adc_val, NULL);
        else
            pcf50635_read_adc(PCF5063X_ADCC1_MUX_BATSNS_RES, &adc_val, NULL);

        current_voltage = (adc_val * BATTERY_SCALE_FACTOR) >> 10;

        last_tick = current_tick;
    }

    return current_voltage;
}
