/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "powermgmt-target.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3659
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3630
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    /* figured from discharge curve */
    { 3631, 3697, 3762, 3784, 3805, 3838, 3902, 3961, 4016, 4091, 4178 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* figured from charge curve */
    3833, 3866, 3910, 3923, 3943, 3969, 4009, 4047, 4095, 4154,  4192
};
