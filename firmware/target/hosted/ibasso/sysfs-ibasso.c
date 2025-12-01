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


#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"

#include "debug-ibasso.h"
#include "sysfs-ibasso.h"

const char* const sysfs_paths[] =
{
    /* SYSFS_DX50_CODEC_VOLUME */
    "/dev/codec_volume",

    /* SYSFS_HOLDKEY */
    "/sys/class/axppower/holdkey",

    /* SYSFS_DX90_ES9018_VOLUME */
    "/sys/class/codec/es9018_volume",

    /* SYSFS_ES9018_FILTER */
    "/sys/class/codec/es9018_filter",

    /* SYSFS_MUTE */
    "/sys/class/codec/mute",

    /* SYSFS_WM8740_MUTE */
    "/sys/class/codec/wm8740_mute",

    /* SYSFS_BATTERY_CAPACITY */
    "/sys/class/power_supply/battery/capacity",

    /* SYSFS_BATTERY_CURRENT_NOW */
    "/sys/class/power_supply/battery/current_now",

    /* SYSFS_BATTERY_ENERGY_FULL_DESIGN */
    "/sys/class/power_supply/battery/energy_full_design",

    /* SYSFS_BATTERY_HEALTH */
    "/sys/class/power_supply/battery/health",

    /* SYSFS_BATTERY_MODEL_NAME */
    "/sys/class/power_supply/battery/model_name",

    /* SYSFS_BATTERY_ONLINE */
    "/sys/class/power_supply/battery/online",

    /* SYSFS_BATTERY_PRESENT */
    "/sys/class/power_supply/battery/present",

    /* SYSFS_BATTERY_STATUS */
    "/sys/class/power_supply/battery/status",

    /* SYSFS_BATTERY_TECHNOLOGY */
    "/sys/class/power_supply/battery/technology",

    /* SYSFS_BATTERY_TEMP */
    "/sys/class/power_supply/battery/temp",

    /* SYSFS_BATTERY_TYPE */
    "/sys/class/power_supply/battery/type",

    /* SYSFS_BATTERY_VOLTAGE_MAX_DESIGN */
    "/sys/class/power_supply/battery/voltage_max_design",

    /* SYSFS_BATTERY_VOLTAGE_MIN_DESIGN */
    "/sys/class/power_supply/battery/voltage_min_design",

    /* SYSFS_BATTERY_VOLTAGE_NOW */
    "/sys/class/power_supply/battery/voltage_now",

    /* SYSFS_USB_POWER_CURRENT_NOW */
    "/sys/class/power_supply/usb/current_now",

    /* SYSFS_USB_POWER_ONLINE */
    "/sys/class/power_supply/usb/online",

    /* SYSFS_USB_POWER_PRESENT */
    "/sys/class/power_supply/usb/present",

    /* SYSFS_USB_POWER_VOLTAGE_NOW */
    "/sys/class/power_supply/usb/voltage_now",

    /* SYSFS_BACKLIGHT_POWER */
    "/sys/devices/platform/rk29_backlight/backlight/rk28_bl/bl_power",

    /* SYSFS_BACKLIGHT_BRIGHTNESS */
    "/sys/devices/platform/rk29_backlight/backlight/rk28_bl/brightness",

    /* SYSFS_POWER_STATE */
    "/sys/power/state",

    /* SYSFS_POWER_WAKE_LOCK */
    "/sys/power/wake_lock"
};
