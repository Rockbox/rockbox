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
#ifndef POWERMGMT_TARGET_H
#define POWERMGMT_TARGET_H

#include "config.h"
#include "powermgmt-imx233.h"

/* Sony NWZ-E360/NWZ-E370 settings:
 * - current ramp slope: 
 * - conditioning threshold voltage:
 * - conditioning max voltage: 
 * - conditioning current:
 * - conditioning timeout: 
 * - charging voltage: 
 * - charging current: 
 * - charging threshold current: 
 * - charging timeout: 
 * - top off period: 
 * - high die temperature: 
 * - low die temperature:
 * - safe die temperature current: 
 * - battery temperature channel: 
 * - high battery temperature: 
 * - low battery temperature: 
 * - safe battery temperature current: 
 * - low DCDC battery voltage: 
 */

#define IMX233_CHARGE_CURRENT   260
#define IMX233_STOP_CURRENT     20
#define IMX233_TOPOFF_TIMEOUT   (30 * 60 * HZ)
#define IMX233_CHARGING_TIMEOUT (4 * 3600 * HZ)
#define IMX233_DIE_TEMP_HIGH    91
#define IMX233_DIE_TEMP_LOW     56
#define IMX233_BATT_TEMP_SENSOR 0
#define IMX233_BATT_TEMP_HIGH   2400
#define IMX233_BATT_TEMP_LOW    2300

#endif /*  POWERMGMT_TARGET_H */
