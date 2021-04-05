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
#include "kernel.h"
#include "axp173.h"
#include "i2c-x1000.h"
#include "gpio-x1000.h"

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

#define AXP173_IRQ_PORT GPIO_B
#define AXP173_IRQ_PIN  (1 << 10)

void power_init(void)
{
    /* Initialize driver */
    i2c_x1000_set_freq(2, I2C_FREQ_400K);
    axp173_init();

    /* Set lowest sample rate */
    axp173_adc_set_rate(AXP173_ADC_RATE_25HZ);

    /* Ensure battery voltage ADC is enabled */
    int bits = axp173_adc_get_enabled();
    bits |= (1 << ADC_BATTERY_VOLTAGE);
    axp173_adc_set_enabled(bits);

    /* Turn on all power outputs */
    i2c_reg_modify1(AXP173_BUS, AXP173_ADDR, 0x12, 0, 0x5f, NULL);
    i2c_reg_modify1(AXP173_BUS, AXP173_ADDR, 0x80, 0, 0xc0, NULL);

    /* Short delay to give power outputs time to stabilize */
    mdelay(5);
}

void adc_init(void)
{
}

void power_off(void)
{
    /* Set the shutdown bit */
    i2c_reg_setbit1(AXP173_BUS, AXP173_ADDR, 0x32, 7, 1, NULL);
    while(1);
}

bool charging_state(void)
{
    return axp173_battery_status() == AXP173_BATT_CHARGING;
}

int _battery_voltage(void)
{
    return axp173_adc_read(ADC_BATTERY_VOLTAGE);
}
