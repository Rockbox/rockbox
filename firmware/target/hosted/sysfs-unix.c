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


#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "debug.h"

#include "sysfs-unix.h"   /* sysfs API header */
#include "sysfs-target.h"   /* platform-specific paths and enums */

static FILE* open_read(const char* file_name)
{
    FILE *f = fopen(file_name, "r");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for reading.", __func__, file_name);
    }

    return f;
}


static FILE* open_write(const char* file_name)
{
    FILE *f = fopen(file_name, "w");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for writing.", __func__, file_name);
    }

    return f;
}


bool sysfs_get_int(enum sys_fs_interface_id id, int* value)
{
    bool success = false;
    const char* interface;

    *value = -1;

    /* check if the provided ID is valid */
    if (id < SYSFS_NUM_INTERFACES)
    {
        interface = SYSFS_PATHS[id];

        FILE *f = open_read(interface);
        if(f == NULL)
        {
            return false;
        }

        if(fscanf(f, "%d", value) == EOF)
        {
            DEBUGF("ERROR %s: Read failed for %s.", __func__, interface);
            success = false;
        }
        else
        {
            success = true;
        }

        fclose(f);
    }

    return success;
}


bool sysfs_set_int(enum sys_fs_interface_id id, int value)
{
    const char* interface;
    bool success = false;

    /* check if the provided ID is valid */
    if (id < SYSFS_NUM_INTERFACES)
    {
        interface = SYSFS_PATHS[id];

        /*DEBUGF("%s: interface: %s, value: %d.", __func__, interface, value);*/

        FILE *f = open_write(interface);
        if(f == NULL)
        {
            return false;
        }

        if(fprintf(f, "%d", value) < 1)
        {
            DEBUGF("ERROR %s: Write failed for %s.", __func__, interface);
            success = false;
        }
        else
        {
            success = true;
        }

        fclose(f);
    }
    else
    {
        DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
    }

    return success;
}


bool sysfs_get_char(enum sys_fs_interface_id id, char* value)
{
    const char* interface;
    bool success = false;

    *value = '\0';

    /* check if the provided ID is valid */
    if (id < SYSFS_NUM_INTERFACES)
    {

        interface = SYSFS_PATHS[id];

        FILE *f = open_read(interface);
        if(f == NULL)
        {
            return false;
        }

        success = true;
        if(fscanf(f, "%c", value) == EOF)
        {
            DEBUGF("ERROR %s: Read failed for %s.", __func__, interface);
            success = false;
        }

        fclose(f);
    }
    else
    {
        DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
    }

    return success;
}


bool sysfs_set_char(enum sys_fs_interface_id id, char value)
{
    const char* interface;
    bool success = false;

    /* check if the provided ID is valid */
    if (id < SYSFS_NUM_INTERFACES)
    {
        interface = SYSFS_PATHS[id];

        FILE *f = open_write(interface);
        if(f == NULL)
        {
            return false;
        }

        success = true;
        if(fprintf(f, "%c", value) < 1)
        {
            DEBUGF("ERROR %s: Write failed for %s.", __func__, interface);
            success = false;
        }

        fclose(f);
    }
    else
    {
        DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
    }

    return success;
}


bool sysfs_get_string(enum sys_fs_interface_id id, char* value, int size)
{
    const char* interface;
    bool success = false;
    
    value[0] = '\0';

    /* check if the provided ID is valid */
    if (id < SYSFS_NUM_INTERFACES)
    {
        success = false;
        interface = SYSFS_PATHS[id];

        /*DEBUGF("%s: interface: %s, size: %d.", __func__, interface, size);*/

        FILE *f = open_read(interface);
        if(f == NULL)
        {
            return false;
        }

        success = true;
        if(fgets(value, size, f) == NULL)
        {
            DEBUGF("ERROR %s: Read failed for %s.", __func__, interface);
            success = false;
        }
        else
        {
            size_t length = strlen(value);
            if((length > 0) && value[length - 1] == '\n')
            {
                value[length - 1] = '\0';
            }
        }

        fclose(f);
    }
    else
    {
        DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
    }
    return success;
}


bool sysfs_set_string(enum sys_fs_interface_id id, char* value)
{
    const char* interface;
    bool success = false;
    
    /* check if the provided ID is valid */
    if (id < SYSFS_NUM_INTERFACES)
    {
        interface = SYSFS_PATHS[id];

        FILE *f = open_write(interface);
        if(f == NULL)
        {
            return false;
        }

        success = true;
        if(fprintf(f, "%s", value) < 1)
        {
            DEBUGF("ERROR %s: Write failed for %s.", __func__, interface);
            success = false;
        }

        fclose(f);
    }
    else
    {
        DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
    }

    return success;
}
