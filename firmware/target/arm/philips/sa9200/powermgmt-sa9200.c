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
    3400
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* Sansa Li Ion 750mAH, took from battery benchs */
    { 3300, 3680, 3740, 3760, 3780, 3810, 3870, 3930, 3970, 4070, 4160 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* Sansa Li Ion 750mAH FIXME */
    3300, 3680, 3740, 3760, 3780, 3810, 3870, 3930, 3970, 4070, 4160
};

/* ADC should read 0x3ff=5.12V */
#define BATTERY_SCALE_FACTOR 5125       
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}

