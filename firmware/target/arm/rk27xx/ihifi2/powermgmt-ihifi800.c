/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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

/*  Battery voltage calculation and discharge/charge curves for the iHiFi 800

    Battery voltage is calculated under the assumption that the adc full-scale
    readout represents 3.00V and that the battery ADC channel is fed with
    exactly half of the battery voltage (through a resistive divider).
    Charge curve have not been calibrated yet.
*/

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* 5% */
    3628,
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* 0% */
    3300,
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3300, 3649, 3701, 3726, 3745, 3778, 3831, 3904, 3965, 4056, 4160 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
    { 3300, 3649, 3701, 3726, 3745, 3778, 3831, 3904, 3965, 4056, 4160 };

/* full-scale ADC readout (2^10) in millivolt */
#define BATTERY_SCALE_FACTOR 6296

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    return (adc_read(ADC_BATTERY) * BATTERY_SCALE_FACTOR) >> 10;
}
