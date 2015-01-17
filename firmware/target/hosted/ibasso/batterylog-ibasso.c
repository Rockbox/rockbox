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
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "debug.h"
#include "rbpaths.h"
#include "tick.h"

#include "batterylog-ibasso.h"
#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


static struct
{
    time_t time;
    int capacity;
    int current_now;
    int energy_full_design;
    char health[30];
    char model_name[30];
    int online;
    int present;
    char status[30];
    char technology[30];
    int temp;
    char type[30];
    int voltage_max_design;
    int voltage_min_design;
    int voltage_now;
    int usb_current_now;
    int usb_online;
    int usb_present;
    int usb_voltage_now;
}
_last_data =
{
    0, -1, -1, -1, { '\0' }, { '\0' }, -1, -1, { '\0' }, { '\0' }, -1, { '\0' }, -1, -1, -1,
    -1, -1, -1, -1
};


static const char LOG_FILE[] = HOME_DIR"/batterylog.csv";


static void batterylog_tick(void)
{
    time_t current_tick = time(NULL);

    if((current_tick - _last_data.time) < 60)
    {
        return;
    }

    _last_data.time = current_tick;

    sysfs_get_int(SYSFS_BATTERY_CAPACITY,    &_last_data.capacity);
    sysfs_get_int(SYSFS_BATTERY_CURRENT_NOW, &_last_data.current_now);
    sysfs_get_int(SYSFS_BATTERY_VOLTAGE_NOW, &_last_data.voltage_now);
    sysfs_get_int(SYSFS_BATTERY_ONLINE,      &_last_data.online);
    sysfs_get_int(SYSFS_BATTERY_PRESENT,     &_last_data.present);
    sysfs_get_string(SYSFS_BATTERY_STATUS, _last_data.status, sizeof(_last_data.status));

    sysfs_get_int(SYSFS_USB_POWER_CURRENT_NOW, &_last_data.usb_current_now);
    sysfs_get_int(SYSFS_USB_POWER_VOLTAGE_NOW, &_last_data.usb_voltage_now);
    sysfs_get_int(SYSFS_USB_POWER_ONLINE,      &_last_data.usb_online);
    sysfs_get_int(SYSFS_USB_POWER_PRESENT,     &_last_data.usb_present);

    FILE* log_file = fopen(LOG_FILE, "a");
    if(log_file == NULL)
    {
        ibasso_enable_batterylog(false);
        DEBUGF("%s: Can not open %s for appendig.", __func__, LOG_FILE);
        return;
    }

    /*
        time [s];capacity [%];current_now [mA];voltage_now [mV];online;present;status;
        usb current_now [mA];usb voltage_now [mV];usb online;usb present
    */
    if(fprintf(log_file,
               "%ld;%d;%d;%d;%d;%d;%s;%d;%d;%d;%d\n",
               _last_data.time,
               _last_data.capacity,
               _last_data.current_now > 0 ? _last_data.current_now / 1000 : _last_data.current_now,
               _last_data.voltage_now > 0 ? _last_data.voltage_now / 1000 : _last_data.voltage_now,
               _last_data.online,
               _last_data.present,
               _last_data.status,
               _last_data.usb_current_now > 0 ? _last_data.usb_current_now / 1000 : _last_data.usb_current_now,
               _last_data.usb_voltage_now > 0 ? _last_data.usb_voltage_now / 1000 : _last_data.usb_voltage_now,
               _last_data.usb_online,
               _last_data.usb_present) < 0)
    {
        DEBUGF("%s: Write failed for %s.", __func__, LOG_FILE);
        ibasso_enable_batterylog(false);
        fclose(log_file);
        return;
    }

    fclose(log_file);
}


/* Default disabled. */
static bool _enabled = false;


void ibasso_enable_batterylog(bool enable)
{
    DEBUGF("DEBUG %s: _enabled: %d, enable: %d.", __func__, _enabled, enable);

    if(enable && (! _enabled))
    {
        _enabled = true;

        FILE* log_file = fopen(LOG_FILE, "a");
        if(log_file == NULL)
        {
            DEBUGF("%s: Can not open %s for appendig.", __func__, LOG_FILE);
            return;
        }

        _last_data.time = time(NULL);

        /* These seem to be constant. */
        sysfs_get_int(SYSFS_BATTERY_ENERGY_FULL_DESIGN, &_last_data.energy_full_design);
        sysfs_get_int(SYSFS_BATTERY_TEMP,               &_last_data.temp);
        sysfs_get_int(SYSFS_BATTERY_VOLTAGE_MAX_DESIGN, &_last_data.voltage_max_design);
        sysfs_get_int(SYSFS_BATTERY_VOLTAGE_MIN_DESIGN, &_last_data.voltage_min_design);
        sysfs_get_string(SYSFS_BATTERY_HEALTH,     _last_data.health,     sizeof(_last_data.health)) ;
        sysfs_get_string(SYSFS_BATTERY_MODEL_NAME, _last_data.model_name, sizeof(_last_data.model_name)) ;
        sysfs_get_string(SYSFS_BATTERY_TECHNOLOGY, _last_data.technology, sizeof(_last_data.technology)) ;
        sysfs_get_string(SYSFS_BATTERY_TYPE,       _last_data.type,       sizeof(_last_data.type));

        if(fprintf(log_file,
                "model_name: %s;type: %s;technology: %s;health: %s;temp: %d;energy_full_design: %d;voltage_min_design: %d;voltage_max_design: %d\n",
                _last_data.model_name,
                _last_data.type,
                _last_data.technology,
                _last_data.health,
                _last_data.temp,
                _last_data.energy_full_design,
                _last_data.voltage_min_design,
                _last_data.voltage_max_design) < 0)
        {
            DEBUGF("%s: Write failed for %s.", __func__, LOG_FILE);
            fclose(log_file);
            return;
        }

        if(fputs("time [s];capacity [%];current_now [mA];voltage_now [mV];online;present;status;usb current_now [mA];usb voltage_now [mV];usb online;usb present\n", log_file) == EOF)
        {
            DEBUGF("%s: Write failed for %s.", __func__, LOG_FILE);
            fclose(log_file);
            return;
        }

        fclose(log_file);

        tick_add_task(batterylog_tick);
    }
    else if(! enable)
    {
        _enabled = false;
        tick_remove_task(batterylog_tick);
    }
}
