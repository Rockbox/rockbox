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

#define SMLAL(lo, hi, x, y)              \
    asm volatile ("smlal %0, %1, %2, %3" \
	 : "+r" (lo), "+r" (hi)              \
	 : "%r" (x), "r" (y))

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3550
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3500
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990 },
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 4200
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    /* return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10; */

    /* This may be overly complicated (pulled from the OF) */
    int lo = 0;
    int val = adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR;

    SMLAL(lo, val, 0x8a42f871, val);
    val>>= 9;
    val -= (val >> 31);
    return val;
}
