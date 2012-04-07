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
#include "adc.h"
#include "sc900776.h"
#include "radio-ypr0.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3470
};

/* the OF shuts down at this voltage */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3450
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3450, 3502, 3550, 3587, 3623, 3669, 3742, 3836, 3926, 4026, 4200 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short const percent_to_volt_charge[11] =
{
      3450, 3670, 3721, 3751, 3782, 3821, 3876, 3941, 4034, 4125, 4200
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
int _battery_voltage(void)
{
    return adc_read(3) * 5;
}

bool charging_state(void)
{
    const unsigned short charged_thres = 4170;
    bool ret = (power_input_status() == POWER_INPUT_MAIN_CHARGER);
    /* dont indicate for > ~95% */
    return ret && (_battery_voltage() <= charged_thres);
}

#if CONFIG_TUNER
static bool tuner_on = false;

bool tuner_power(bool status)
{
    if (status != tuner_on)
    {
        tuner_on = status;
        status = !status;
        if (tuner_on) {
            radiodev_open();
        }
        else {
            radiodev_close();
        }
    }

    return status;
}

bool tuner_powered(void)
{
    return tuner_on;
}
#endif /* #if CONFIG_TUNER */