/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 Andrew Ryabinin
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

/*  Battery voltage calculation and discharge/charge curves for the HiFiMAN HM-801.

    I don't know how to calculate battery voltage, so all values represented here is just
    values from adc battery channel, not millivolts.
    Discharge and charge curves have not been calibrated yet.
*/

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /* OF power off device when this value reached  */
    430
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    425
};

/* adc values of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* TODO: simple uncalibrated curve */
    { 425, 430, 440, 450, 460, 470,  480, 490, 500, 510, 520 }
};

/* adc values of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
    /* TODO: simple uncalibrated curve */
    { 425, 430, 440, 450, 460, 470,  480, 490, 500, 510, 520  };

int _battery_voltage(void)
{
    return adc_read(ADC_BATTERY);
}
