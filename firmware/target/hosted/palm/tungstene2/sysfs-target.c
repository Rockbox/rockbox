/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 Lorenzo Miori
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

/* Path definitions */
const char* SYSFS_PATHS[] =
{
    /* SYSFS_BACKLIGHT_POWER */
    "/sys/devices/platform/pxa25x-pwm.0/pwm-backlight.0/backlight/pwm-backlight.0/bl_power",

    /* SYSFS_BACKLIGHT_BRIGHTNESS */
    "/sys/devices/platform/pxa25x-pwm.0/pwm-backlight.0/backlight/pwm-backlight.0/brightness",

	/* SYSFS_POWER_AC */
	"/sys/class/power_supply/ac/online",

	/* SYSFS_POWER_BATTERY_VOLTAGE */
	"/sys/class/power_supply/main-batt/voltage_now",

	/* SYSFS_POWER_BATTERY_TEMP */
	"/sys/class/power_supply/main-batt/temp",

};
