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

#include "axp-2101.h"
#include "power.h"
#include "system.h"
#include "i2c-async.h"
#include <string.h>

/* Headers for the debug menu */
#ifndef BOOTLOADER
# include "action.h"
# include "list.h"
# include <stdio.h>
#endif

struct axp_adc_info {
    uint8_t reg;
    uint8_t en_reg;
    uint8_t en_bit;
    int8_t num;
    int8_t den;
};

struct axp_supply_info {
    uint8_t volt_reg;
    uint8_t volt_reg_mask;
    uint8_t en_reg;
    uint8_t en_bit;
    int min_mV;
    int max_mV; // if multiple steps, set to max of step 1
    int step_mV;
    int step2_min_mV;
    int step2_mV;
    int step2_max_mV;
    int step3_min_mV;
    int step3_mV;
    int step3_max_mV;
};

static const struct axp_adc_info axp_adc_info[AXP2101_NUM_ADC_CHANNELS] = {
    // 0x000    0x001   0x002 ... 0x1FFF
    // 0mV      1mV     2mV   ... 8.192V
    [AXP2101_ADC_VBAT_VOLTAGE]      = {AXP2101_REG_ADC_VBAT_H,  AXP2101_REG_ADCCHNENABLE, 1 << 0, 1, 1},
    // 0mV      1mV     2mV   ... 8.192V
    [AXP2101_ADC_VBUS_VOLTAGE]      = {AXP2101_REG_ADC_VBUS_H,  AXP2101_REG_ADCCHNENABLE, 1 << 2, 1, 1},
    // 0mV      1mV     2mV   ... 8.192V
    [AXP2101_ADC_VSYS_VOLTAGE]      = {AXP2101_REG_ADC_VSYS_H,  AXP2101_REG_ADCCHNENABLE, 1 << 3, 1, 1},
    // 0mV      0.5mV   0.1mV   ... 4.096V
    [AXP2101_ADC_TS_VOLTAGE]        = {AXP2101_REG_ADC_TS_H,    AXP2101_REG_ADCCHNENABLE, 1 << 1, 1, 2},
    // see axp2101_adc_conv_raw() for conversion
    [AXP2101_ADC_DIE_TEMPERATURE]   = {AXP2101_REG_ADC_TDIE_H,  AXP2101_REG_ADCCHNENABLE, 1 << 4, 1, 1},
};

static const struct axp_supply_info axp_supply_info[AXP2101_NUM_SUPPLIES] = {
    [AXP2101_SUPPLY_DCDC1] = {
        .volt_reg = 0x82,
        .volt_reg_mask = 0x1f, // N.B. max value 0b10011, values higher reserved
        .en_reg = 0x80,
        .en_bit = 0,
        .min_mV = 1500,
        .max_mV = 3400,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_DCDC2] = {
        .volt_reg = 0x83,
        .volt_reg_mask = 0x7f, // N.B. max value 0b1010111, values higher reserved
        .en_reg = 0x80,
        .en_bit = 1,
        .min_mV = 500,
        .max_mV = 1200,
        .step_mV = 10,
        .step2_min_mV = 1220,
        .step2_mV = 20,
        .step2_max_mV = 1540,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
        // N.B. 10mV/step from 0.5 - 1.2v (71 steps),
        //      20mV/step from 1.22 - 1.54v (17 steps)
    },
    [AXP2101_SUPPLY_DCDC3] = {
        .volt_reg = 0x84,
        .volt_reg_mask = 0x7f, // N.B. max value 0b1101011, values higher reserved
        .en_reg = 0x80,
        .en_bit = 2,
        .min_mV = 500,
        .max_mV = 1200,
        .step_mV = 10,
        .step2_min_mV = 1220,
        .step2_mV = 20,
        .step2_max_mV = 1540,
        .step3_min_mV = 1600,
        .step3_mV = 100,
        .step3_max_mV = 3400,
        // N.B. 10mV/step from 0.5 - 1.2V (71 steps)
        //      20mV/step from 1.22 - 1.54V (17 steps)
        //      100mV/step from 1.6 - 3.4V (19 steps)
    },
    [AXP2101_SUPPLY_DCDC4] = {
        .volt_reg = 0x85,
        .volt_reg_mask = 0x7f, // N.B. max value 0b1100110, values higher reserved
        .en_reg = 0x80,
        .en_bit = 3,
        .min_mV = 500,
        .max_mV = 1200,
        .step_mV = 10,
        .step2_min_mV = 1220,
        .step2_mV = 20,
        .step2_max_mV = 1840,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
        // N.B. 10mV/step from 0.5 - 1.2V (71 steps)
        //      20mV/step from 1.22 - 1.84V (32 steps)
    },
    [AXP2101_SUPPLY_DCDC5] = {
        .volt_reg = 0x86,
        .volt_reg_mask = 0x3f, // N.B. max value 0b10111, values higher reserved
        .en_reg = 0x80,
        .en_bit = 4,
        .min_mV = 1400,
        .max_mV = 3700,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_ALDO1] = {
        .volt_reg = 0x92,
        .volt_reg_mask = 0x3f, // N.B. max value 0b1110, values higher reserved
        .en_reg = 0x90,
        .en_bit = 0,
        .min_mV = 500,
        .max_mV = 3500,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_ALDO2] = {
        .volt_reg = 0x93,
        .volt_reg_mask = 0x3f, // N.B. max value 0b1110, values higher reserved
        .en_reg = 0x90,
        .en_bit = 1,
        .min_mV = 500,
        .max_mV = 3500,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_ALDO3] = {
        .volt_reg = 0x94,
        .volt_reg_mask = 0x3f, // N.B. max value 0b1110, values higher reserved
        .en_reg = 0x90,
        .en_bit = 2,
        .min_mV = 500,
        .max_mV = 3500,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_ALDO4] = {
        .volt_reg = 0x95,
        .volt_reg_mask = 0x3f, // N.B. max value 0b1110, values higher reserved
        .en_reg = 0x90,
        .en_bit = 3,
        .min_mV = 500,
        .max_mV = 3500,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_BLDO1] = {
        .volt_reg = 0x96,
        .volt_reg_mask = 0x3f, // N.B. max value 0b11110, values higher reserved
        .en_reg = 0x90,
        .en_bit = 4,
        .min_mV = 500,
        .max_mV = 3500,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_BLDO2] = {
        .volt_reg = 0x97,
        .volt_reg_mask = 0x3f, // N.B. max value 0b11110, values higher reserved
        .en_reg = 0x90,
        .en_bit = 5,
        .min_mV = 500,
        .max_mV = 3500,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_DLDO1] = {
        .volt_reg = 0x99,
        .volt_reg_mask = 0x3f, // N.B. max value 0b11100, values higher reserved
        .en_reg = 0x90,
        .en_bit = 7,
        .min_mV = 500,
        .max_mV = 3400,
        .step_mV = 100,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_DLDO2] = {
        .volt_reg = 0x9a,
        .volt_reg_mask = 0x3f, // N.B. max value 0b11100, values higher reserved
        .en_reg = 0x91,
        .en_bit = 0,
        .min_mV = 500,
        .max_mV = 1400,
        .step_mV = 560,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    [AXP2101_SUPPLY_VCPUS] = {
        .volt_reg = 0x98,
        .volt_reg_mask = 0x3f, // N.B. max value 0b10011, values higher reserved
        .en_reg = 0x90,
        .en_bit = 6,
        .min_mV = 500,
        .max_mV = 1400,
        .step_mV = 50,
        .step2_min_mV = 0,
        .step2_mV = 0,
        .step2_max_mV = 0,
        .step3_min_mV = 0,
        .step3_mV = 0,
        .step3_max_mV = 0,
    },
    // No voltage reg given - are these fixed?
    // [AXP_SUPPLY_RTCLDO1] = {
    // },
    // [AXP_SUPPLY_RTCLDO2] = {
    // },
};

void axp2101_init(void)
{
}

void axp2101_supply_set_voltage(int supply, int voltage)
{
    const struct axp_supply_info* info = &axp_supply_info[supply];
    if(info->volt_reg == 0 || info->volt_reg_mask == 0)
        return;

    if(voltage > 0 && info->step_mV != 0) {
        if(voltage < info->min_mV)
            return;

        int regval;

        // there's probably a more elegant way to do this...
        if(voltage > info->max_mV) {
            if(info->step2_max_mV == 0) {
                return;
            } else {
                if(voltage > info->step2_max_mV) {
                    if(info->step3_max_mV == 0 || voltage > info->step3_max_mV) {
                        return;
                    } else {
                        // step3 range
                        regval = ((info->max_mV - info->min_mV) / info->step_mV)\
                                        + ((info->step2_max_mV - info->step2_min_mV) / info->step2_mV)\
                                        + ((voltage - info->step3_min_mV) / info->step3_mV) + 2;
                    }
                } else {
                    // step2 range
                    regval = ((info->max_mV - info->min_mV) / info->step_mV)\
                                    + ((voltage - info->step2_min_mV) / info->step2_mV) + 1;
                }
            }
        } else {
            // step1 range
            regval = (voltage - info->min_mV) / info->step_mV;
        }
        i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR, info->volt_reg,
                        info->volt_reg_mask, regval, NULL);
    }

    if(info->en_bit != 0xff) {
        i2c_reg_setbit1(AXP_PMU_BUS, AXP_PMU_ADDR,
                        info->en_reg, info->en_bit,
                        voltage > 0 ? 1 : 0, NULL);
    }
}

int axp2101_supply_get_voltage(int supply)
{
    const struct axp_supply_info* info = &axp_supply_info[supply];
    if(info->volt_reg == 0)
        return AXP2101_SUPPLY_NOT_PRESENT;

    if(info->en_reg != 0) {
        int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, info->en_reg);
        if(r < 0)
            return AXP2101_SUPPLY_DISABLED;
        if(r & (1 << info->en_bit) == 0)
            return AXP2101_SUPPLY_DISABLED;
    }

    /* Hack, avoid undefined shift below. Can be useful too... */
    if(info->volt_reg_mask == 0)
        return info->min_mV;

    int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, info->volt_reg);
    if(r < 0)
        return 0;

    int val;
    r = r & info->volt_reg_mask;

    // there's probably a more elegant way to do this...
    if(r > ((info->max_mV - info->min_mV) / info->step_mV)) {
        r = r - ((info->max_mV - info->min_mV) / info->step_mV);

        if(r > ((info->step2_max_mV - info->step2_min_mV) / info->step2_mV + 1)) {
            r = r - ((info->step2_max_mV - info->step2_min_mV) / info->step2_mV);
            /* step 3 */
            val = info->step3_min_mV + ((r-2) * info->step3_mV);

        } else {
            /* step 2 */
            val = info->step2_min_mV + ((r-1) * info->step2_mV);

        }
    } else {
        /* step 1 */
        val = info->min_mV + (r * info->step_mV);

    }

    return val;
}

int axp2101_battery_status(void)
{
    int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_PMU_STATUS2);
    if(((r >> 5) & 0x03) == 0) {
        return AXP2101_BATT_FULL;
    } else if(((r >> 5) & 0x03) == 1) {
        return AXP2101_BATT_CHARGING;
    } else {
        return AXP2101_BATT_DISCHARGING;
    }
}

int axp2101_input_status(void)
{
#ifdef HAVE_BATTERY_SWITCH
    int input_status = 0;
#else
    int input_status = AXP2101_INPUT_BATTERY;
#endif

    int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_PMU_STATUS1);
    if(r & 0x20)
        input_status |= AXP2101_INPUT_USB;
#ifdef HAVE_BATTERY_SWITCH
    if(r & 0x80)
        input_status |= AXP2101_INPUT_BATTERY;
#endif

    return input_status;
}

int axp2101_adc_read(int adc)
{
    int value = axp2101_adc_read_raw(adc);
    if(value == INT_MIN)
        return INT_MIN;

    return axp2101_adc_conv_raw(adc, value);
}

int axp2101_adc_read_raw(int adc)
{
    /* Read the ADC */
    uint8_t buf[2];
    uint8_t reg = axp_adc_info[adc].reg;
    int rc = i2c_reg_read(AXP_PMU_BUS, AXP_PMU_ADDR, reg, 2, &buf[0]);
    if(rc != I2C_STATUS_OK)
        return INT_MIN;

    /* Parse the value */
    return ((int)(buf[0] & 0x3f) << 8) | (buf[1] & 0xff);
}

int axp2101_adc_conv_raw(int adc, int value)
{
    if (adc == AXP2101_ADC_DIE_TEMPERATURE)
        return 22 + ((7274 - value) / 20);

    // seems to be a signed 14-bit value, but
    // let's clamp it to zero. Seems to give sane results
    // on v_bus and v_ts channels
    if (value & 0x2000)
        return 0;

    return axp_adc_info[adc].num * value / axp_adc_info[adc].den;
}

void axp2101_adc_set_enabled(int adc_bits)
{
    uint8_t xfer[1];
    xfer[0] = 0;

    /* Compute the new register values */
    const struct axp_adc_info* info = axp_adc_info;
    for(int i = 0; i < AXP2101_NUM_ADC_CHANNELS; ++i) {
        if(!(adc_bits & (1 << i)))
            continue;

        xfer[0] |= info[i].en_bit;
    }

    /* Update the configuration */
    i2c_reg_write(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_ADCCHNENABLE, 1, &xfer[0]);
}

int axp2101_egauge_read(void)
{
    uint8_t buf;
    i2c_reg_read(AXP_PMU_BUS, AXP_PMU_ADDR, AXP2101_REG_BATT_PERCENTAGE, 1, &buf);
    return (int)buf;
}

// there are many current settings:
// Reg 16: Input current limit control
// Reg 61: Precharge current limit
// Reg 62: Constant current charge current limit
// Reg 63: Charging termination current limit

// there are also voltage settings for charging:
// Reg 14: Linear Charger Vsys voltage dpm
// Reg 15: Input Voltage limit control
// Reg 64: CV charger charge voltage limit

// There are also some timer stuff:
// Reg 67: Charger timeout setting and control

// constant current charge current limits
static const int chargecurrent_tbl[] = {
    0,  25, 50, 75, 100, 125, 150, 175, 200,
    300,  400, 500,  600,  700, 800,  900, 1000,
};

// constant current charge current limits
void axp2101_set_charge_current(int current_mA)
{
    /* find greatest charging current not exceeding requested current */
    unsigned int index = 0;
    while(index < ARRAYLEN(chargecurrent_tbl)-1 &&
          chargecurrent_tbl[index+1] <= current_mA)
        ++index;

    i2c_reg_modify1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP2101_REG_ICC_SETTING, 0x0f, index, NULL);
}

int axp2101_get_charge_current(void)
{
    int ret = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR,
                            AXP2101_REG_ICC_SETTING);
    if(ret < 0)
        ret = 0;

    return chargecurrent_tbl[ret & 0x0f];
}

void axp2101_power_off(void)
{
    /* Set the shutdown bit */
    i2c_reg_setbit1(AXP_PMU_BUS, AXP_PMU_ADDR,
                    AXP2101_REG_PMUCOMMCONFIG, 0, 1, NULL);
}

#ifndef BOOTLOADER
enum {
    AXP_DEBUG_BATTERY_STATUS,
    AXP_DEBUG_INPUT_STATUS,
    AXP_DEBUG_CHARGE_CURRENT,
    AXP_DEBUG_EGAUGE_VALUE,
    AXP_DEBUG_FIRST_ADC,
    AXP_DEBUG_FIRST_SUPPLY = AXP_DEBUG_FIRST_ADC + AXP2101_NUM_ADC_CHANNELS,
    AXP_DEBUG_NUM_ENTRIES = AXP_DEBUG_FIRST_SUPPLY + AXP2101_NUM_SUPPLIES,
};

static int axp2101_debug_menu_cb(int action, struct gui_synclist* lists)
{
    (void)lists;

    if(action == ACTION_NONE)
        action = ACTION_REDRAW;

    return action;
}

static const char* axp2101_debug_menu_get_name(int item, void* data,
                                           char* buf, size_t buflen)
{
    (void)data;

    static const char* const adc_names[] = {
        "V_bat", "V_bus", "V_sys", "V_ts", "T_die",
    };

    static const char* const adc_units[] = {
        "mV", "mV", "mV", "mV", "C",
    };

    static const char* const supply_names[] = {
        "DCDC1", "DCDC2", "DCDC3", "DCDC4", "DCDC5",
        "ALDO1", "ALDO2", "ALDO3", "ALDO4", "BLDO1", "BLDO2", "DLDO1", "DLDO2",
        "VCPUS",
    };

    int adc = item - AXP_DEBUG_FIRST_ADC;
    if(item >= AXP_DEBUG_FIRST_ADC && adc < AXP2101_NUM_ADC_CHANNELS) {
        int raw_value = axp2101_adc_read_raw(adc);
        if(raw_value == INT_MIN) {
            snprintf(buf, buflen, "%s: [Disabled]", adc_names[adc]);
            return buf;
        }

        int value = axp2101_adc_conv_raw(adc, raw_value);
        snprintf(buf, buflen, "%s: %d %s", adc_names[adc],
                    value, adc_units[adc]);

        return buf;
    }

    int supply = item - AXP_DEBUG_FIRST_SUPPLY;
    if(item >= AXP_DEBUG_FIRST_SUPPLY && supply < AXP2101_NUM_SUPPLIES) {
        int voltage = axp2101_supply_get_voltage(supply);
        if(voltage == AXP2101_SUPPLY_NOT_PRESENT)
            snprintf(buf, buflen, "%s: [Not Present]", supply_names[supply]);
        else if(voltage == AXP2101_SUPPLY_DISABLED)
            snprintf(buf, buflen, "%s: [Disabled]", supply_names[supply]);
        else
            snprintf(buf, buflen, "%s: %d mV", supply_names[supply], voltage);

        return buf;
    }

    switch(item) {
    case AXP_DEBUG_BATTERY_STATUS: {
        switch(axp2101_battery_status()) {
        case AXP2101_BATT_FULL:
            return "Battery: Full";
        case AXP2101_BATT_CHARGING:
            return "Battery: Charging";
        case AXP2101_BATT_DISCHARGING:
            return "Battery: Discharging";
        default:
            return "Battery: Unknown";
        }
    } break;

    case AXP_DEBUG_INPUT_STATUS: {
        int s = axp2101_input_status();
        const char* ac = (s & AXP2101_INPUT_AC) ? " AC" : "";
        const char* usb = (s & AXP2101_INPUT_USB) ? " USB" : "";
        const char* batt = (s & AXP2101_INPUT_BATTERY) ? " Battery" : "";
        snprintf(buf, buflen, "Inputs:%s%s%s", ac, usb, batt);
        return buf;
    } break;

    case AXP_DEBUG_CHARGE_CURRENT: {
        int current = axp2101_get_charge_current();
        snprintf(buf, buflen, "Max charge current: %d mA", current);
        return buf;
    } break;

    case AXP_DEBUG_EGAUGE_VALUE: {
        int percent = axp2101_egauge_read();
        snprintf(buf, buflen, "EGauge percent: %d", percent);
        return buf;
    } break;

    default:
        return "---";
    }
}

bool axp2101_debug_menu(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "AXP debug", AXP_DEBUG_NUM_ENTRIES, NULL);
    info.action_callback = axp2101_debug_menu_cb;
    info.get_name = axp2101_debug_menu_get_name;
    return simplelist_show_list(&info);
}
#endif /* !BOOTLOADER */

/* This is basically the only valid implementation, so define it here */
unsigned int axp2101_power_input_status(void)
{
    unsigned int state = 0;
    int input_status = axp2101_input_status();

    if(input_status & AXP2101_INPUT_AC)
        state |= POWER_INPUT_MAIN_CHARGER;

    if(input_status & AXP2101_INPUT_USB)
        state |= POWER_INPUT_USB_CHARGER;

#ifdef HAVE_BATTERY_SWITCH
    if(input_status & AXP2101_INPUT_BATTERY)
        state |= POWER_INPUT_BATTERY;
#endif

    return state;
}
