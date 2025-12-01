/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#ifndef _SYSFS_IBASSO_H_
#define _SYSFS_IBASSO_H_

#include "sysfs.h"

/*
    Sys FS path identifiers.
    See SYSFS_PATHS in sysfs-ibasso.c.
*/
enum sys_fs_interface_id
{
    SYSFS_DX50_CODEC_VOLUME = 0,
    SYSFS_HOLDKEY,
    SYSFS_DX90_ES9018_VOLUME,
    SYSFS_ES9018_FILTER,
    SYSFS_MUTE,
    SYSFS_WM8740_MUTE,
    SYSFS_BATTERY_CAPACITY,
    SYSFS_BATTERY_CURRENT_NOW,
    SYSFS_BATTERY_ENERGY_FULL_DESIGN,
    SYSFS_BATTERY_HEALTH,
    SYSFS_BATTERY_MODEL_NAME,
    SYSFS_BATTERY_ONLINE,
    SYSFS_BATTERY_PRESENT,
    SYSFS_BATTERY_STATUS,
    SYSFS_BATTERY_TECHNOLOGY,
    SYSFS_BATTERY_TEMP,
    SYSFS_BATTERY_TYPE,
    SYSFS_BATTERY_VOLTAGE_MAX_DESIGN,
    SYSFS_BATTERY_VOLTAGE_MIN_DESIGN,
    SYSFS_BATTERY_VOLTAGE_NOW,
    SYSFS_USB_POWER_CURRENT_NOW,
    SYSFS_USB_POWER_ONLINE,
    SYSFS_USB_POWER_PRESENT,
    SYSFS_USB_POWER_VOLTAGE_NOW,
    SYSFS_BACKLIGHT_POWER,
    SYSFS_BACKLIGHT_BRIGHTNESS,
    SYSFS_POWER_STATE,
    SYSFS_POWER_WAKE_LOCK
};

extern const char* const sysfs_paths[];

#endif
