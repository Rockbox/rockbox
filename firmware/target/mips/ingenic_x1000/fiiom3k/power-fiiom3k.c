/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#ifdef HAVE_USB_CHARGING_ENABLE
# include "usb_core.h"
#endif
#include "axp-192.h"
#include "i2c-ingenic.h"

unsigned short battery_level_disksafe = 3470;

/* the OF shuts down at this voltage */
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

void power_init(void)
{
    /* Initialize driver */
    i2c_ingenic_set_freq(2, I2C_FREQ_400K);

    /* Set lowest sample rate */
    axp192_adc_set_rate(AXP192_ADC_RATE_25HZ);

    /* Enable required ADCs */
    axp192_adc_set_enabled(
        (1 << AXP192_ADC_BATTERY_VOLTAGE) |
        (1 << AXP192_ADC_CHARGE_CURRENT) |
        (1 << AXP192_ADC_DISCHARGE_CURRENT) |
        (1 << AXP192_ADC_VBUS_VOLTAGE) |
        (1 << AXP192_ADC_VBUS_CURRENT) |
        (1 << AXP192_ADC_INTERNAL_TEMP) |
        (1 << AXP192_ADC_APS_VOLTAGE));

    /*
     * Drop VCORE voltage to the standard 1200mV, for some reason the
     * default is 1400mV which is unnecessarily high and outside the
     * recommended operating range.
     */
    axp192_supply_set_voltage(AXP192_SUPPLY_DCDC2, 1200);

    /* Turn on all power outputs */
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP192_REG_PWROUTPUTCTRL2, 0, 0x5f, NULL);
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP192_REG_DCDCWORKINGMODE, 0, 0xc0, NULL);

    /* Set the default charging current. This is the same as the
     * OF's setting, although it's not strictly within the USB spec. */
    axp192_set_charge_current(780);

    /* Short delay to give power outputs time to stabilize */
    mdelay(200);
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    axp192_set_charge_current(maxcurrent);
}
#endif

void adc_init(void)
{
}

void power_off(void)
{
    axp192_power_off();
    while(1);
}

bool charging_state(void)
{
    return axp192_battery_status() == AXP192_BATT_CHARGING;
}

unsigned int power_input_status(void)
{
    return axp192_power_input_status();
}

int _battery_voltage(void)
{
    return axp192_adc_read(AXP192_ADC_BATTERY_VOLTAGE);
}

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
int _battery_current(void)
{
    if(charging_state())
        return axp192_adc_read(AXP192_ADC_CHARGE_CURRENT);
    else
        return axp192_adc_read(AXP192_ADC_DISCHARGE_CURRENT);
}
#endif
