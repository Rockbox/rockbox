/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Hairo R. Carela
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
#include "powermgmt.h"
#include "power.h"

/* System handles powering off at 2% */
unsigned short battery_level_disksafe = 0;
unsigned short battery_level_shutoff = 0;

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
/* read from /sys/class/power_supply/axp20x-battery/voltage_now */
/* in 5s intervals and averaged out */
unsigned short percent_to_volt_discharge[11] =
{
    3400, 3597, 3665, 3701, 3736, 3777, 3824, 3882, 3944, 4023, 4162
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
{
    3512, 3729, 3795, 3831, 3865, 3906, 3953, 4010, 4072, 4150, 4186
};
