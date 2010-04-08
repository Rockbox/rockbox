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
#include "powermgmt.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    /*
     * about 10%, calibrated with C240v2 battery profile at
     * http://www.rockbox.org/wiki/bin/viewfile/Main/SansaRuntime?filename=c240v2_battery_bench_percent.png;rev=3
     */
    3600
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3300
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /*
     * Below table is calibrated according to
     * http://www.rockbox.org/wiki/bin/viewfile/Main/SansaRuntime?filename=c240v2_battery_bench_percent.png;rev=3
     * OF seems to stop charging at 4150mV, so that's 100% here.
     */
    { 3300, 3597, 3674, 3719, 3745, 3776, 3825, 3890, 3954, 4035, 4150 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* TODO: simple uncalibrated curve with 10% knee at 3.60V */
    3400, 3600, 3670, 3740, 3810, 3880, 3950, 4020, 4090, 4160, 4230
};
