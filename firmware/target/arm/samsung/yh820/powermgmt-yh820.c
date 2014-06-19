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
#include "adc.h"
#include "powermgmt.h"


const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3200
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3000
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
/* NOTE: readout clips at around 4000mV  */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3200, 3550, 3610, 3680, 3710, 3740, 3800, 3880, 3910, 3995, 4150 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
/* NOTE: these values may be rather inaccurate. Readout clips at around 4000mV */
const unsigned short percent_to_volt_charge[11] =
{
      3750, 3860, 3880, 3900, 3930, 3994, 4080, 4135, 4200, 4200, 4200
};

#define BATTERY_SCALE_FACTOR 5000

/* Returns battery voltage from ADC [millivolts] */
/* NOTE: readout clips at 3995mV in normal operation/3898mV without PLL (idle mode) */
int _battery_voltage(void)
{
    /* overall error vs. voltmeter is < 1.5% */
    return ((adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10) - 1097;
}
