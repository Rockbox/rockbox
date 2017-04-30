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

#ifndef _SYSFS_TARGET_H_
#define _SYSFS_TARGET_H_

/*
    Sys FS path identifiers.
    See SYSFS_PATHS in sysfs-ibasso.c.
*/
enum sys_fs_interface_id
{
    SYSFS_BACKLIGHT_POWER = 0,
    SYSFS_BACKLIGHT_BRIGHTNESS,
	SYSFS_POWER_AC,
	SYSFS_POWER_BATTERY_VOLTAGE,
	SYSFS_POWER_BATTERY_TEMP,

    /* do not remove - last item */
    SYSFS_NUM_INTERFACES
};

extern const char* SYSFS_PATHS[];

#endif  /* _SYSFS_TARGET_H_ */
