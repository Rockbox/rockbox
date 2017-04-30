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
 * Copyright (C) 2015 by Lorenzo Miori: API generalized for multi-platform support
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
#include "sysfs-target.h"

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
