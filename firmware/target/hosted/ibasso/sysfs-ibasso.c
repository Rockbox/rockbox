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


static const char* const SYSFS_PATHS[] =
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


static FILE* open_read(const char* file_name)
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


bool sysfs_get_int(enum sys_fs_interface_id id, int* value)
{
    *value = -1;

    switch(id)
    {
        case SYSFS_BATTERY_CAPACITY:
        case SYSFS_BATTERY_CURRENT_NOW:
        case SYSFS_BATTERY_ENERGY_FULL_DESIGN:
        case SYSFS_BATTERY_ONLINE:
        case SYSFS_BATTERY_PRESENT:
        case SYSFS_BATTERY_TEMP:
        case SYSFS_BATTERY_VOLTAGE_MAX_DESIGN:
        case SYSFS_BATTERY_VOLTAGE_MIN_DESIGN:
        case SYSFS_BATTERY_VOLTAGE_NOW:
        case SYSFS_USB_POWER_CURRENT_NOW:
        case SYSFS_USB_POWER_VOLTAGE_NOW:
        case SYSFS_USB_POWER_ONLINE:
        case SYSFS_USB_POWER_PRESENT:
        {
            break;
        }

        default:
        {
            DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
            return false;
        }
    }

    const char* interface = SYSFS_PATHS[id];

    /*DEBUGF("%s: interface: %s.", __func__, interface);*/

    FILE *f = open_read(interface);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fscanf(f, "%d", value) == EOF)
    {
        DEBUGF("ERROR %s: Read failed for %s.", __func__, interface);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_set_int(enum sys_fs_interface_id id, int value)
{
    switch(id)
    {
        case SYSFS_BACKLIGHT_POWER:
        case SYSFS_BACKLIGHT_BRIGHTNESS:
        case SYSFS_DX50_CODEC_VOLUME:
        case SYSFS_DX90_ES9018_VOLUME:
        {
            break;
        }

        default:
        {
            DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
            return false;
        }
    }

    const char* interface = SYSFS_PATHS[id];

    /*DEBUGF("%s: interface: %s, value: %d.", __func__, interface, value);*/

    FILE *f = open_write(interface);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fprintf(f, "%d", value) < 1)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, interface);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_get_char(enum sys_fs_interface_id id, char* value)
{
    *value = '\0';

    switch(id)
    {
        case SYSFS_HOLDKEY:
        {
            break;
        }

        default:
        {
            DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
            return false;
        }
    }

    const char* interface = SYSFS_PATHS[id];

    /*DEBUGF("%s: interface: %s.", __func__, interface);*/

    FILE *f = open_read(interface);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fscanf(f, "%c", value) == EOF)
    {
        DEBUGF("ERROR %s: Read failed for %s.", __func__, interface);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_set_char(enum sys_fs_interface_id id, char value)
{
    switch(id)
    {
        case SYSFS_MUTE:
        case SYSFS_WM8740_MUTE:
        case SYSFS_ES9018_FILTER:
        {
            break;
        }

        default:
        {
            DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
            return false;
        }
    }

    const char* interface = SYSFS_PATHS[id];

    /*DEBUGF("%s: interface: %s, value: %c.", __func__, interface, value);*/

    FILE *f = open_write(interface);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fprintf(f, "%c", value) < 1)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, interface);
        success = false;
    }

    fclose(f);
    return success;
}


bool sysfs_get_string(enum sys_fs_interface_id id, char* value, int size)
{
    value[0] = '\0';

    switch(id)
    {
        case SYSFS_BATTERY_STATUS:
        case SYSFS_BATTERY_HEALTH:
        case SYSFS_BATTERY_MODEL_NAME:
        case SYSFS_BATTERY_TECHNOLOGY:
        case SYSFS_BATTERY_TYPE:
        {
            break;
        }

        default:
        {
            DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
            return false;
        }
    }

    const char* interface = SYSFS_PATHS[id];

    /*DEBUGF("%s: interface: %s, size: %d.", __func__, interface, size);*/

    FILE *f = open_read(interface);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
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
    return success;
}


bool sysfs_set_string(enum sys_fs_interface_id id, char* value)
{
    switch(id)
    {
        case SYSFS_POWER_STATE:
        case SYSFS_POWER_WAKE_LOCK:
        {
            break;
        }

        default:
        {
            DEBUGF("ERROR %s: Unknown interface id: %d.", __func__, id);
            return false;
        }
    }

    const char* interface = SYSFS_PATHS[id];

    /*DEBUGF("%s: interface: %s, value: %s.", __func__, interface, value);*/

    FILE *f = open_write(interface);
    if(f == NULL)
    {
        return false;
    }

    bool success = true;
    if(fprintf(f, "%s", value) < 1)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, interface);
        success = false;
    }

    fclose(f);
    return success;
}
