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
#if   defined(IRIVER_H10)
    3733
#elif defined(IRIVER_H10_5GB)
    3700
#endif
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
#if   defined(IRIVER_H10)
    3627
#elif defined(IRIVER_H10_5GB)
    3600
#endif
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
#if   defined(IRIVER_H10)
    { 3733, 3772, 3821, 3840, 3869, 3917, 3985, 4034, 4072, 4140, 4198 }
#elif defined(IRIVER_H10_5GB)
    { 3700, 3800, 3850, 3880, 3910, 3960, 4000, 4070, 4120, 4210, 4280 }
#endif
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
#if   defined(IRIVER_H10)
    3956, 3995, 4024, 4043, 4063, 4082, 4111, 4140, 4179, 4218, 4266
#elif defined(IRIVER_H10_5GB)
    /* TODO: Not yet calibrated */
    3700, 3800, 3850, 3880, 3910, 3960, 4000, 4070, 4120, 4210, 4280
#endif
};

#define BATTERY_SCALE_FACTOR 4650
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}
