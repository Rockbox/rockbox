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
#if   defined(IPOD_NANO)
    3330
#elif defined(IPOD_VIDEO)
    3300
#elif defined(IPOD_COLOR)
    3300
#elif defined(IPOD_3G)
    3700
#else
    /* FIXME: calibrate value for other iPods */
    3300
#endif
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
#if   defined(IPOD_NANO)
    3230
#elif defined(IPOD_VIDEO)
    3300
#elif defined(IPOD_COLOR)
    3300
#elif defined(IPOD_3G)
    3500
#else
    /* FIXME: calibrate value for other iPods */
    3000
#endif
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
#if   defined(IPOD_NANO)
    /* measured values */
    { 3230, 3620, 3700, 3730, 3750, 3780, 3830, 3890, 3950, 4030, 4160 },
#elif defined(IPOD_VIDEO)
    /* iPod Video 30GB Li-Ion 400mAh, first approach based upon measurements */
    { 3450, 3670, 3710, 3750, 3790, 3830, 3870, 3930, 4010, 4100, 4180 },
#elif defined(IPOD_COLOR)
    /* iPod Photo 30GB, see FS#9072 */
    { 3450, 3660, 3700, 3730, 3750, 3770, 3820, 3870, 3920, 4040, 4170 },
#elif defined(IPOD_3G)
    /* iPod 3G 40GB, first approach based upon measurements */
    { 3720, 3740, 3760, 3780, 3830, 3870, 3910, 3970, 4020, 4060, 4090 },
#else
    /* FIXME: calibrate value for other iPods */
    /* Table is "provisional" from IPOD_COLOR */
    { 3450, 3660, 3700, 3730, 3750, 3770, 3820, 3870, 3920, 4040, 4170 }
#endif
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
#if   defined(IPOD_NANO)
    /* measured values */
    3230, 3620, 3700, 3730, 3750, 3780, 3830, 3890, 3950, 4030, 4160
#elif defined(IPOD_VIDEO)
    /* iPOD Video 30GB Li-Ion 400mAh, first approach based upon measurements */
    3450, 3670, 3710, 3750, 3790, 3830, 3870, 3930, 4010, 4100, 4180
#elif defined(IPOD_COLOR)
    /* iPod Photo 30GB, see FS#9072 */
    3450, 3660, 3700, 3730, 3750, 3770, 3820, 3870, 3920, 4040, 4170
#elif defined(IPOD_3G)
    /* iPod 3G 40GB, first approach based upon measurements */
    3720, 3740, 3760, 3780, 3830, 3870, 3910, 3970, 4020, 4060, 4090
#else
    /* FIXME: calibrate value for other iPods */
    /* Table is "provisional" from IPOD_COLOR */
    3450, 3660, 3700, 3730, 3750, 3770, 3820, 3870, 3920, 4040, 4170
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
        /* Accessory voltage supply on */
        pcf50605_write(PCF5060X_D2REGC1, 0xf8); /* 3.3V ON */
    }
    else
    {
        /* Accessory voltage supply off */
        pcf50605_write(PCF5060X_D2REGC1, 0x18); /* OFF */
    }

}
#endif
