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
#include "pcf5060x.h"
#include "pcf50605.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
#ifdef IPOD_NANO
    3330
#elif defined IPOD_VIDEO
    3300
#else
    /* FIXME: calibrate value for other 3G+ ipods */
    3380
#endif
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
#ifdef IPOD_NANO
    3230
#elif defined IPOD_VIDEO
    3300
#else
    /* FIXME: calibrate value for other 3G+ ipods */
    3020
#endif
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
#ifdef IPOD_NANO
    /* measured values */
    { 3230, 3620, 3700, 3730, 3750, 3780, 3830, 3890, 3950, 4030, 4160 },
#elif defined IPOD_VIDEO
    /* iPOD Video 30GB Li-Ion 400mAh, first approach based upon measurements */
    { 3450, 3670, 3710, 3750, 3790, 3830, 3870, 3930, 4010, 4100, 4180 },
#else
    /* FIXME: calibrate value for other 3G+ ipods */
    /* Table is "inherited" from iriver H100. */
    { 3370, 3650, 3700, 3740, 3780, 3820, 3870, 3930, 4000, 4080, 4160 }
#endif
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
#ifdef IPOD_NANO
    /* measured values */
    3230, 3620, 3700, 3730, 3750, 3780, 3830, 3890, 3950, 4030, 4160
#elif defined IPOD_VIDEO
    /* iPOD Video 30GB Li-Ion 400mAh, first approach based upon measurements */
    3450, 3670, 3710, 3750, 3790, 3830, 3870, 3930, 4010, 4100, 4180
#else
    /* FIXME: calibrate value for other 3G+ ipods */
    /* Table is "inherited" from iriver H100. */
    3540, 3860, 3930, 3980, 4000, 4020, 4040, 4080, 4130, 4180, 4230
#endif
};
#endif /* CONFIG_CHARGING */

#define BATTERY_SCALE_FACTOR 6000
/* full-scale ADC readout (2^10) in millivolt */

/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    return (adc_read(ADC_UNREG_POWER) * BATTERY_SCALE_FACTOR) >> 10;
}

#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool enable)
{
    if (enable)
    {
        /* Accessory voltage supply */
        pcf50605_write(PCF5060X_D2REGC1, 0xf8); /* 3.3V ON */
    }
    else
    {
        /* Accessory voltage supply */
        pcf50605_write(PCF5060X_D2REGC1, 0x18); /* OFF */
    }
    
}
#endif
