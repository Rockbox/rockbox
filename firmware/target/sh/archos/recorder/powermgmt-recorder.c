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
    4750
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    4400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* original values were taken directly after charging, but it should show
       100% after turning off the device for some hours, too */
    { 4500, 4810, 4910, 4970, 5030, 5070, 5120, 5140, 5170, 5250, 5400 }
                                            /* orig. values: ...,5280,5600 */
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* values guessed, see
       http://www.seattlerobotics.org/encoder/200210/LiIon2.pdf until someone
       measures voltages over a charging cycle */
    4760, 5440, 5510, 5560, 5610, 5640, 5660, 5760, 5820, 5840, 5850 /* NiMH */
};

#define BATTERY_SCALE_FACTOR 6620
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}
