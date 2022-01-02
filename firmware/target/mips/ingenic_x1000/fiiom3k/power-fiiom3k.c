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
#ifdef HAVE_USB_CHARGING_ENABLE
# include "usb_core.h"
#endif
#include "axp192.h"
#include "i2c-x1000.h"

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
    /* Configure I2C bus */
    i2c_x1000_set_freq(AXP_PMU_BUS, I2C_FREQ_400K);

    /* Set DCDC1 and DCDC2 to fixed PWM mode to match OF settings. */
    axp_modify(AXP_REG_DCDCMODE, 0, 0x0c);

    /* Power on required supplies */
    axp_set_enabled_supplies(
        (1 << AXP_SUPPLY_DCDC1) | /* not sure (3.3 V) */
        (1 << AXP_SUPPLY_DCDC2) | /* not sure (1.4 V) */
        (1 << AXP_SUPPLY_DCDC3) | /* for CPU (1.8 V) */
        (1 << AXP_SUPPLY_LDO2) |  /* LCD controller (3.3 V) */
        (1 << AXP_SUPPLY_LDO3));  /* SD bus (3.3 V) */

    /* Enable required ADCs */
    axp_set_enabled_adcs(
        (1 << AXP_ADC_BATTERY_VOLTAGE) |
        (1 << AXP_ADC_CHARGE_CURRENT) |
        (1 << AXP_ADC_DISCHARGE_CURRENT) |
        (1 << AXP_ADC_VBUS_VOLTAGE) |
        (1 << AXP_ADC_VBUS_CURRENT) |
        (1 << AXP_ADC_INTERNAL_TEMP) |
        (1 << AXP_ADC_APS_VOLTAGE));

    /* Configure USB charging */
    axp_set_vhold_level(4400);
    usb_charging_maxcurrent_change(100);

    /* Short delay to give power outputs time to stabilize */
    mdelay(200);
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    int vbus_limit;
    int charge_current;

    /* Note that the charge current setting is a maximum: it will be
     * reduced dynamically by the AXP192 so the combined load is less
     * than the set VBUS current limit. */
    if(maxcurrent <= 100) {
        vbus_limit = AXP_VBUS_LIMIT_500mA;
        charge_current = 100;
    } else {
        vbus_limit = AXP_VBUS_LIMIT_500mA;
        charge_current = 550;
    }

    axp_set_vbus_limit(vbus_limit);
    axp_set_charge_current(charge_current);
}
#endif

void adc_init(void)
{
}

void power_off(void)
{
    axp_power_off();
    while(1);
}

bool charging_state(void)
{
    return axp_is_charging();
}

unsigned int power_input_status(void)
{
    return axp_power_input_status();
}

int _battery_voltage(void)
{
    return axp_read_adc(AXP_ADC_BATTERY_VOLTAGE);
}

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
int _battery_current(void)
{
    if(charging_state())
        return axp_read_adc(AXP_ADC_CHARGE_CURRENT);
    else
        return axp_read_adc(AXP_ADC_DISCHARGE_CURRENT);
}
#endif
