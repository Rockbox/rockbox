/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

/* TODO
    Each as3525 target should probably define its own battery properties
    (dangerous/shutoff battery level, discharge/charge curves) in a file
    called powermgmt-<target>.c in its own sub-directory.
    
    For now, this file provides simple uncalibrated settings to get at least
    a basic reading for all as3525 targets with a lithium battery.
 */

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* TODO: this is just an initial guess */
    3400
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    /* TODO: this is just an initial guess */
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* TODO: simple uncalibrated curve, linear except for first 10% */
    { 3300, 3390, 3480, 3570, 3660, 3750, 3840, 3930, 4020, 4110, 4200 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* TODO: simple uncalibrated curve, linear except for first 10% */
    3300, 3390, 3480, 3570, 3660, 3750, 3840, 3930, 4020, 4110, 4200
};
#endif /* CONFIG_CHARGING */

/* ADC should read 0x3ff=5.12V */
#define BATTERY_SCALE_FACTOR 5125       
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_RTCSUP) * BATTERY_SCALE_FACTOR) >> 10;
}

