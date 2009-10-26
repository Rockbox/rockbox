/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bob Cousins
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
#include "system.h"
#include "adc.h"
#include "power.h"
#include "powermgmt.h"

/* The following constants are dummy values since there is no battery */
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3450
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* Typical Li Ion 830mAH  */
    { 3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* Typical Li Ion 830mAH */
    3480, 3550, 3590, 3610, 3630, 3650, 3700, 3760, 3800, 3910, 3990
};


/* Returns battery voltage from ADC [millivolts] */
/* full-scale (2^10) in millivolt */
unsigned int battery_adc_voltage(void)
{
    /* Since we have no battery, return a fully charged value */
    return 4000 * 1024 / 1000;
}

unsigned int input_millivolts(void)
{
    unsigned int batt_millivolts = battery_voltage();

   /* No battery, return nominal value */
    return batt_millivolts;
}


