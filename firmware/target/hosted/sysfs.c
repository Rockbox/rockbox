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
#include "sysfs.h"


static FILE* open_read(const char *file_name)
{
    FILE *f = fopen(file_name, "re");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for reading.", __func__, file_name);
    }

    return f;
}


static FILE* open_write(const char* file_name)
{
    FILE *f = fopen(file_name, "we");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for writing.", __func__, file_name);
    }

    return f;
}


bool sysfs_get_int(const char *path, int *value)
{
    *value = -1;

    FILE *f = open_read(path);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fscanf(f, "%d", value) == EOF)
    {
        DEBUGF("ERROR %s: Read failed for %s.", __func__, path);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_set_int(const char *path, int value)
{
    FILE *f = open_write(path);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fprintf(f, "%d", value) < 0)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, path);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_get_char(const char *path, char *value)
{
    int c;
    FILE *f = open_read(path);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    c = fgetc(f);

    if(c == EOF)
    {
        DEBUGF("ERROR %s: Read failed for %s.", __func__, path);
        success = false;
    }
    else
    {
        *value = c;
    }

    fclose(f);
    return success;
}


bool sysfs_set_char(const char *path, char value)
{
    FILE *f = open_write(path);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fprintf(f, "%c", value) < 1)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, path);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_get_string(const char *path, char *value, int size)
{
    value[0] = '\0';
    FILE *f = open_read(path);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;

    /* fgets returns NULL if en error occured OR
     * when EOF occurs while no characters have been read.
     *
     * Empty string is not an error for us.
     */
    if(fgets(value, size, f) == NULL && value[0] != '\0')
    {
        DEBUGF("ERROR %s: Read failed for %s.", __func__, path);
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
    return success;
}


bool sysfs_set_string(const char *path, char *value)
{
    FILE *f = open_write(path);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;

    /* If an output error is encountered, a negative value is returned */
    if(fprintf(f, "%s", value) < 0)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, path);
        success = false;
    }

    fclose(f);
    return success;
}
