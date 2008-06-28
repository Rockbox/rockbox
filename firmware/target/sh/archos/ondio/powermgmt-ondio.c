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
    3100, 3450
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    2700, 2800
};

/* voltages (millivolt) of 0%, 10%, ... 100% */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* measured values */
    { 2800, 3250, 3410, 3530, 3640, 3740, 3850, 3950, 4090, 4270, 4750 }, /* Alkaline */
    { 3100, 3550, 3630, 3690, 3720, 3740, 3760, 3780, 3800, 3860, 4050 }  /* NiMH */
};

#define BATTERY_SCALE_FACTOR 4849 /* average from 3 Ondios */
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}

