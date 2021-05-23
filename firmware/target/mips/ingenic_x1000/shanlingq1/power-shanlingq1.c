/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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
#include "adc.h"
#include "system.h"
#include "axp-pmu.h"
#include "i2c-x1000.h"
#ifdef HAVE_USB_CHARGING_ENABLE
# include "usb_core.h"
#endif

// FIXME(q1): implement
const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3470
};

/* the OF shuts down at this voltage */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3400, 3639, 3697, 3723, 3757, 3786, 3836, 3906, 3980, 4050, 4159 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
      3485, 3780, 3836, 3857, 3890, 3930, 3986, 4062, 4158, 4185, 4196
};

void power_init(void)
{
    i2c_x1000_set_freq(AXP_PMU_BUS, I2C_FREQ_400K);
    axp_init();

    /* Change supply voltage from the default of 1250 mV to 1200 mV,
     * this matches the original firmware's settings. Didn't observe
     * any obviously bad behavior at 1250 mV, but better to be safe. */
    axp_supply_set_voltage(AXP_SUPPLY_DCDC2, 1200);

    /* For now, just turn everything on... definitely the touchscreen
     * is powered by one of the outputs */
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP_REG_PWROUTPUTCTRL1, 0, 0x05, NULL);
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP_REG_PWROUTPUTCTRL2, 0, 0x0f, NULL);
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP_REG_DCDCWORKINGMODE, 0, 0xc0, NULL);

    /* Delay to give power output time to stabilize. */
    mdelay(50);
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    (void)maxcurrent;
    // FIXME(q1): implement
}
#endif

void power_off(void)
{
    axp_power_off();
    while(1);
}

void adc_init(void)
{
    // FIXME(q1): implement
}

bool charging_state(void)
{
    // FIXME(q1): implement
    return false;
}

int _battery_voltage(void)
{
    // FIXME(q1): implement
    return 4200;
}
