/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2013 Andrew Ryabinin
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

/**
 * Battery voltage calculation and discharge/charge curves for the HiFi E.T MA9.
 */

unsigned short battery_level_disksafe = 6700; /* TODO: this is just an initial guess */

unsigned short battery_level_shutoff = 6600;

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
    /* Uncalibrated curve */
    6600, 6936, 7042, 7124, 7218, 7288, 7382, 7534, 7674, 7838, 8200
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
    /* TODO: simple copy of discharge curve */
    { 6600, 6936, 7042, 7124, 7218, 7288, 7382, 7534, 7674, 7838, 8200 };

/* full-scale ADC readout (2^10) in millivolt */
#define BATTERY_SCALE_FACTOR  9170

/* Returns battery voltage from ADC [millivolts] */
int _battery_voltage(void)
{
    return (adc_read(ADC_BATTERY) * BATTERY_SCALE_FACTOR) >> 10;
}
