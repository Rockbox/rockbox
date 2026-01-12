/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "power.h"
#include "mutex.h"
#include "gpio-stm32h7.h"

static struct mutex power_1v8_lock;
static int power_1v8_refcount;

unsigned short battery_level_disksafe = 3500;
unsigned short battery_level_shutoff = 3400;

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
    3400, 3639, 3697, 3723, 3757, 3786, 3836, 3906, 3980, 4050, 4159
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
{
    3485, 3780, 3836, 3857, 3890, 3930, 3986, 4062, 4158, 4185, 4196
};

/* API for enabling 1V8 regulator */
void echoplayer_enable_1v8_regulator(bool enable)
{
    mutex_lock(&power_1v8_lock);

    if (enable)
    {
        if (power_1v8_refcount++ == 0)
            gpio_set_level(GPIO_POWER_1V8, 1);
    }
    else
    {
        if (--power_1v8_refcount == 0)
            gpio_set_level(GPIO_POWER_1V8, 0);
    }

    mutex_unlock(&power_1v8_lock);
}

/* Power management */
void power_init(void)
{
    mutex_init(&power_1v8_lock);
}

void power_off(void)
{
    gpio_set_level(GPIO_CPU_POWER_ON, 0);

    /* TODO: reset to bootloader if USB is plugged in */
    while (1)
        core_idle();
}

void system_reboot(void)
{
    /*
     * TODO: support reboot
     *
     * For R1-Rev1 PCBs doing a CPU reset will cut power when
     * running on battery (because cpu_power_on is no longer
     * being driven high). The RTC alarm could be used to wake
     * the system instead.
     */
    power_off();
}

unsigned int power_input_status(void)
{
    return POWER_INPUT_USB_CHARGER;
}

bool charging_state(void)
{
    return true;
}

int _battery_voltage(void)
{
    return 4000;
}
