/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: powermgmt-sim.c 29543 2011-03-08 19:33:30Z thomasjfox $
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
#include "config.h"
#include <sys/ioctl.h>
#include "kernel.h"
#include "powermgmt.h"
#include "power.h"
#include "file.h"
#include "ascodec-target.h"
#include "as3514.h"
#include "sc900776.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3500
};

/* the OF shuts down at this voltage */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3450
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
/* FIXME: This is guessed. Make proper curve using battery_bench */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3450, 3692, 3740, 3772, 3798, 3828, 3876, 3943, 4013, 4094, 4194 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
/* FIXME: This is guessed. Make proper curve using battery_bench */
const unsigned short const percent_to_volt_charge[11] =
{
    3600, 3802, 3856, 3888, 3905, 3931, 3973, 4025, 4084, 4161, 4219
};

unsigned int power_input_status(void)
{
    unsigned status = POWER_INPUT_NONE;
    int fd = open("/dev/minivet", O_RDONLY);
    if (fd >= 0)
    {
        if (ioctl(fd, IOCTL_MINIVET_DET_VBUS, NULL) > 0)
            status = POWER_INPUT_MAIN_CHARGER;
        close(fd);
    }
    return status;
}

#endif /* CONFIG_CHARGING */


/* Returns battery voltage from ADC [millivolts],
 * adc returns voltage in 5mV steps */
unsigned int battery_adc_voltage(void)
{
    return adc_read(3) * 5;
}

bool charging_state(void)
{
    /* cannot make this static (initializer not constant error), but gcc
     * seems to calculate at compile time anyway */
    const unsigned short charged_thres =
        ((percent_to_volt_charge[9] + percent_to_volt_charge[10]) / 2);

    bool ret = (power_input_status() == POWER_INPUT_MAIN_CHARGER);
    /* dont indicate for > ~95% */
    return ret && (battery_adc_voltage() <= charged_thres);
}
