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
#ifndef POWERMGMT_TARGET_H
#define POWERMGMT_TARGET_H

#include "config.h"
#include "powermgmt-imx233.h"

/* Creative ZEN X-Fi3 OF settings:
 * - current ramp slope: 10 mA/s
 * - conditioning threshold voltage: 3 V
 * - conditioning max voltage: 3.1 V
 * - conditioning current: 60 mA
 * - conditioning timeout: 1 h
 * - charging voltage: 4.2 V
 * - charging current: 260 mA
 * - charging threshold current: 20 mA
 * - charging timeout: 4 h
 * - top off period: 30 min
 * - high die temperature: 91 °C
 * - low die temperature: 56 °C
 * - safe die temperature current: 30 mA
 * - battery temperature channel: 0
 * - high battery temperature: 2400
 * - low battery temperature: 2300
 * - safe battery temperature current: 0 mA
 * - low DCDC battery voltage: 3.9 V
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
