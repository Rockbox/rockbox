/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
    /* Toshiba Gigabeat S Li Ion 700mAH figured from discharge curve */
    { 3659, 3719, 3745, 3761, 3785, 3813, 3856, 3926, 3984, 4040, 4121 },
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    /* Toshiba Gigabeat S Li Ion 700mAH figured from charge curve */
    4028, 4063, 4087, 4111, 4135, 4156, 4173, 4185, 4194, 4202, 4208
};
