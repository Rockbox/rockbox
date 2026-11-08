/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "power.h"
#include "adc.h"
#include "system.h"
#include "kernel.h"
#ifdef HAVE_USB_CHARGING_ENABLE
# include "usb_core.h"
#endif
#include "axp-192.h"
#include "axp-2101.h"
#include "i2c-ingenic.h"
#include "power-erosqnative.h"

int eros_qn_pmu_type = -1;

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

static void power_init_axp2101(void)
{
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

#ifndef BOOTLOADER
    /* delay to allow ADC to get a good sample -
     * may give bogus (high) readings otherwise. */
    mdelay(150);
#endif

#ifdef BOOTLOADER
    /* Delay to give power outputs time to stabilize.
     * With the power thread delay, this can apparently go as low as 50,
     * Keeping a higher value here just to ensure the bootloader works
     * correctly. */
    mdelay(200);
#endif
}

static void power_init_axp192(void)
{
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

    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP192_REG_PWROUTPUTCTRL2, 0, 0x5f, NULL);
    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP192_REG_DCDCWORKINGMODE, 0, 0xc0, NULL);

    /* Set the default charging current. This is the same as the
     * OF's setting, although it's not strictly within the USB spec. */
    axp192_set_charge_current(780);

#ifdef BOOTLOADER
    /* Delay to give power outputs time to stabilize. */
    mdelay(200);
#endif
}

void power_init(void)
{
    /* Initialize driver */
    i2c_ingenic_set_freq(2, I2C_FREQ_400K);

    /* Detect PMU type */
    for (int tries = 0; tries < 3; ++tries)
    {
        /* chip ID register is common to AXP192 and AXP2101 */
        int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP192_REG_CHIP_ID);
        if (r >= 0)
        {
            eros_qn_pmu_type = r;
            break;
        }
    }

    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP192:
        power_init_axp192();
        break;
    case EROS_QN_PMU_AXP2101:
        power_init_axp2101();
        break;
    default:
        /* Don't do anything if we don't know what PMU is used */
        break;
    }
}

#ifdef HAVE_USB_CHARGING_ENABLE
void usb_charging_maxcurrent_change(int maxcurrent)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        axp2101_set_charge_current(maxcurrent);
        break;
    case EROS_QN_PMU_AXP192:
        axp192_set_charge_current(maxcurrent);
        break;
    }
}
#endif

void adc_init(void)
{
}

void power_off(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        axp2101_power_off();
        break;
    case EROS_QN_PMU_AXP192:
        axp192_power_off();
        break;
    }

    while(1);
}

bool charging_state(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        return axp2101_battery_status() == AXP2101_BATT_CHARGING;
    case EROS_QN_PMU_AXP192:
        return axp192_battery_status() == AXP192_BATT_CHARGING;
    default:
        return false;
    }
}

unsigned int power_input_status(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        return axp2101_power_input_status();
    case EROS_QN_PMU_AXP192:
        return axp192_power_input_status();
    default:
        return false;
    }
}

int _battery_level(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        return axp2101_egauge_read();
    default:
        return -1;
    }
}

int _battery_voltage(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        return axp2101_adc_read(AXP2101_ADC_VBAT_VOLTAGE);
    case EROS_QN_PMU_AXP192:
        return axp192_adc_read(AXP192_ADC_BATTERY_VOLTAGE);
    default:
        return 4000; /* fake it so we don't immediately power off */
    }
}

#if CONFIG_BATTERY_MEASURE & CURRENT_MEASURE
int _battery_current(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP192:
        if (charging_state())
            return axp192_adc_read(AXP192_ADC_CHARGE_CURRENT);
        else
            return axp192_adc_read(AXP192_ADC_DISCHARGE_CURRENT);

    case EROS_QN_PMU_AXP2101:
        /* not supported here */
        return -1;

    default:
        return -1;
    }
}
#endif

bool erosqn_power_debug_menu(void)
{
    switch (eros_qn_pmu_type)
    {
    case EROS_QN_PMU_AXP2101:
        return axp2101_debug_menu();
    case EROS_QN_PMU_AXP192:
        return axp192_debug_menu();
    default:
        return false;
    }
}
