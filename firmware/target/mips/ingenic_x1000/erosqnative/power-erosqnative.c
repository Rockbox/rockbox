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
#include "axp-2101.h"
#include "i2c-x1000.h"
#include "devicedata.h"

unsigned short battery_level_disksafe = 3470;

/* The OF shuts down at this voltage */
unsigned short battery_level_shutoff = 3400;

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
unsigned short percent_to_volt_discharge[11] =
{
    3400, 3477, 3540, 3578, 3617, 3674, 3771, 3856, 3936, 4016, 4117
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
unsigned short percent_to_volt_charge[11] =
{
      3400, 3477, 3540, 3578, 3617, 3674, 3771, 3856, 3936, 4016, 4117
};

void power_init(void)
{
    /* Initialize driver */
    i2c_x1000_set_freq(2, I2C_FREQ_400K);

    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever >= 4){
        uint8_t regread;
        axp2101_init();
        /* Enable required ADCs */
        axp2101_adc_set_enabled(
            (1 << AXP2101_ADC_VBAT_VOLTAGE) |
            (1 << AXP2101_ADC_VBUS_VOLTAGE) |
            (1 << AXP2101_ADC_VSYS_VOLTAGE) |
            (1 << AXP2101_ADC_VBUS_VOLTAGE) |
            (1 << AXP2101_ADC_TS_VOLTAGE) |
            (1 << AXP2101_ADC_DIE_TEMPERATURE));

        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP2101_REG_DCDC_ONOFF, 0, 0x1f, NULL);
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP2101_REG_LDO_ONOFF0, 0, 0xff, NULL);
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP2101_REG_LDO_ONOFF1, 0, 0x01, NULL);

        // set power button delay to 1s to match earlier devices
        regread = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP2101_REG_LOGICTHRESH);
        if ((regread&0x03) != 0x02){
            i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                            AXP2101_REG_LOGICTHRESH, 0x03, 0, NULL);
            i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                            AXP2101_REG_LOGICTHRESH, 0, 0x02, NULL);
        }
        
        // These match the OF as far as I can discern
        axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC1, 3300);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC2, 1200);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC3, 2800);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC4, 1800);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_DCDC5, 1400);

        axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO1, 2500);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO2, 3300);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO3, 3300);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_ALDO4, 3300);

        axp2101_supply_set_voltage(AXP2101_SUPPLY_BLDO1, 3300);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_BLDO2, 3300);

        axp2101_supply_set_voltage(AXP2101_SUPPLY_DLDO1, 2500);
        axp2101_supply_set_voltage(AXP2101_SUPPLY_DLDO2, 1250);

        axp2101_supply_set_voltage(AXP2101_SUPPLY_VCPUS, 500);

        /* Set the default charging current. This is the same as the
        * OF's setting, although it's not strictly within the USB spec. */
        axp2101_set_charge_current(780);

        /* delay to allow ADC to get a good sample -
         * may give bogus (high) readings otherwise. */
#ifndef BOOTLOADER
        mdelay(150);
#endif
    } else {
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
        
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP_REG_PWROUTPUTCTRL2, 0, 0x5f, NULL);
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        AXP_REG_DCDCWORKINGMODE, 0, 0xc0, NULL);

        /* Set the default charging current. This is the same as the
        * OF's setting, although it's not strictly within the USB spec. */
        axp_set_charge_current(780);
    }

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
    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever >= 4){
        axp2101_set_charge_current(maxcurrent);
    } else {
        axp_set_charge_current(maxcurrent);
    }
    
}
#endif

void adc_init(void)
{
}

void power_off(void)
{
    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever >= 4){
        axp2101_power_off();
    } else {
        axp_power_off();
    }
    while(1);
}

bool charging_state(void)
{
    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever >= 4){
        return axp2101_battery_status() == AXP2101_BATT_CHARGING;
    } else {
        return axp_battery_status() == AXP_BATT_CHARGING;
    }
}

int _battery_level(void)
{
    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever >= 4){
        return axp2101_egauge_read();
    } else {
        return -1;
    }
}

int _battery_voltage(void)
{
    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever >= 4){
        return axp2101_adc_read(AXP2101_ADC_VBAT_VOLTAGE);
    } else {
        return axp_adc_read(ADC_BATTERY_VOLTAGE);
    }
}

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
int _battery_current(void)
{
    int devicever;
#if defined(BOOTLOADER)
    devicever = EROSQN_VER;
#else
    devicever = device_data.hw_rev;
#endif
    if (devicever <= 3){
        if(charging_state())
            return axp_adc_read(ADC_CHARGE_CURRENT);
        else
            return axp_adc_read(ADC_DISCHARGE_CURRENT);
    }
}
#endif
