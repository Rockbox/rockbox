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
#include "pcf50606.h"

unsigned short current_voltage = 3910;

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* FIXME: calibrate value */
    3380
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* FIXME: calibrate value */
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* FIXME: calibrate values. Table is "inherited" from iPod-PCF / H100 */
    { 3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* FIXME: calibrate values. Table is "inherited" from iPod-PCF / H100 */
    3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 6000
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return current_voltage;
}

