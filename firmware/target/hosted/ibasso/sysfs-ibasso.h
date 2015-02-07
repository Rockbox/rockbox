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


#include <stdbool.h>


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


/*
    Read a integer value from the sys fs interface given by id.
    Returns true on success, false else.
*/
bool sysfs_get_int(enum sys_fs_interface_id id, int* value);


/*
    Write a integer value to the sys fs interface given by id.
    Returns true on success, false else.
*/
bool sysfs_set_int(enum sys_fs_interface_id id, int value);


/*
    Read a char value from the sys fs interface given by id.
    Returns true on success, false else.
*/
bool sysfs_get_char(enum sys_fs_interface_id id, char* value);


/*
    Write a char value to the sys fs interface given by id.
    Returns true on success, false else.
*/
bool sysfs_set_char(enum sys_fs_interface_id id, char value);

/*
    Read a single line of text from the sys fs interface given by id.
    A newline will be discarded.
    size: The size of value.
    Returns true on success, false else.
*/
bool sysfs_get_string(enum sys_fs_interface_id id, char* value, int size);


/*
    Write text to the sys fs interface given by id.
    Returns true on success, false else.
*/
bool sysfs_set_string(enum sys_fs_interface_id id, char* value);


#endif
