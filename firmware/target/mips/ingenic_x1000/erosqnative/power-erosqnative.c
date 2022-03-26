/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald, Dana Conrad
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

// whole file copied from m3k

#include "power.h"
#include "adc.h"
#include "system.h"
#include "kernel.h"
#ifdef HAVE_USB_CHARGING_ENABLE
# include "usb_core.h"
#endif
#include "axp-pmu.h"
#include "i2c-x1000.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3470
};

/* The OF shuts down at this voltage */
const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3400
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3400, 3477, 3540, 3578, 3617, 3674, 3771, 3856, 3936, 4016, 4117 }
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
      3400, 3477, 3540, 3578, 3617, 3674, 3771, 3856, 3936, 4016, 4117
};

void power_init(void)
{
    /* Initialize driver */
    i2c_x1000_set_freq(2, I2C_FREQ_400K);
    axp_init();

    /* Set lowest sample rate */
    axp_adc_set_rate(AXP_ADC_RATE_25HZ);

    /* Enable required ADCs */
    axp_adc_set_enabled(
        (1 << ADC_BATTERY_VOLTAGE) |
        (1 << ADC_CHARGE_CURRENT) |
        (1 << ADC_DISCHARGE_CURRENT) |
        (1 << ADC_VBUS_VOLTAGE) |
        (1 << ADC_VBUS_CURRENT) |
        (1 << ADC_INTERNAL_TEMP) |
        (1 << ADC_APS_VOLTAGE));

    /* Turn on all power outputs */
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP_REG_PWROUTPUTCTRL2, 0, 0x5f, NULL);
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP_REG_DCDCWORKINGMODE, 0, 0xc0, NULL);

    /* Set the default charging current. This is the same as the
     * OF's setting, although it's not strictly within the USB spec. */
    axp_set_charge_current(780);

#ifdef BOOTLOADER
    /* Delay to give power outputs time to stabilize.
     * With the power thread delay, this can apparently go as low as 50,
     * Keeping a higher value here just to ensure the bootloader works
     * correctly. */
    mdelay(200);
#endif
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    axp_set_charge_current(maxcurrent);
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
    return axp_battery_status() == AXP_BATT_CHARGING;
}

int _battery_voltage(void)
{
    return axp_adc_read(ADC_BATTERY_VOLTAGE);
}

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
int _battery_current(void)
{
    if(charging_state())
        return axp_adc_read(ADC_CHARGE_CURRENT);
    else
        return axp_adc_read(ADC_DISCHARGE_CURRENT);
}
#endif
