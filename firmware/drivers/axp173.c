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

#include "axp173.h"
#include "power.h"
#include "i2c-async.h"

/* Headers for the debug menu */
#ifndef BOOTLOADER
# include "action.h"
# include "list.h"
# include <stdio.h>
#endif

static const struct axp173_adc_info {
    uint8_t reg;
    uint8_t en_reg;
    uint8_t en_bit;
} axp173_adc_info[NUM_ADC_CHANNELS] = {
    {0x56, 0x82, 5},    /* ACIN_VOLTAGE */
    {0x58, 0x82, 4},    /* ACIN_CURRENT */
    {0x5a, 0x82, 3},    /* VBUS_VOLTAGE */
    {0x5c, 0x82, 2},    /* VBUS_CURRENT */
    {0x5e, 0x83, 7},    /* INTERNAL_TEMP */
    {0x62, 0x82, 1},    /* TS_INPUT */
    {0x78, 0x82, 7},    /* BATTERY_VOLTAGE */
    {0x7a, 0x82, 6},    /* CHARGE_CURRENT */
    {0x7c, 0x82, 6},    /* DISCHARGE_CURRENT */
    {0x7e, 0x82, 1},    /* APS_VOLTAGE */
    {0x70, 0xff, 0},    /* BATTERY_POWER */
};

static struct axp173 {
    int adc_enable;
} axp173;

static void axp173_init_enabled_adcs(void)
{
    axp173.adc_enable = 0;

    /* Read enabled ADCs from the hardware */
    uint8_t regs[2];
    int rc = i2c_reg_read(AXP173_BUS, AXP173_ADDR, 0x82, 2, &regs[0]);
    if(rc != I2C_STATUS_OK)
        return;

    /* Parse registers to set ADC enable bits */
    const struct axp173_adc_info* info = axp173_adc_info;
    for(int i = 0; i < NUM_ADC_CHANNELS; ++i) {
        if(info[i].en_reg == 0xff)
            continue;

        if(regs[info[i].en_reg - 0x82] & info[i].en_bit)
            axp173.adc_enable |= 1 << i;
    }

    /* Handle battery power ADC */
    if((axp173.adc_enable & (1 << ADC_BATTERY_VOLTAGE)) &&
       (axp173.adc_enable & (1 << ADC_DISCHARGE_CURRENT))) {
        axp173.adc_enable |= (1 << ADC_BATTERY_POWER);
    }
}

void axp173_init(void)
{
    axp173_init_enabled_adcs();

    /* We need discharge current ADC to reliably poll for a full battery */
    int bits = axp173.adc_enable;
    bits |= (1 << ADC_DISCHARGE_CURRENT);
    axp173_adc_set_enabled(bits);
}

/* TODO: this can STILL indicate some false positives! */
int axp173_battery_status(void)
{
    int r = i2c_reg_read1(AXP173_BUS, AXP173_ADDR, 0x00);
    if(r >= 0) {
        /* Charging bit indicates we're currently charging */
        if((r & 0x04) != 0)
            return AXP173_BATT_CHARGING;

        /* Not plugged in means we're discharging */
        if((r & 0xf0) == 0)
            return AXP173_BATT_DISCHARGING;
    } else {
        /* Report discharging if we can't find out power status */
        return AXP173_BATT_DISCHARGING;
    }

    /* If the battery is full and not in use, the charging bit will be 0,
     * there will be an external power source, AND the discharge current
     * will be zero. Seems to rule out all false positives. */
    int d = axp173_adc_read_raw(ADC_DISCHARGE_CURRENT);
    if(d == 0)
        return AXP173_BATT_FULL;

    return AXP173_BATT_DISCHARGING;
}

int axp173_input_status(void)
{
#ifdef HAVE_BATTERY_SWITCH
    int input_status = 0;
#else
    int input_status = AXP173_INPUT_BATTERY;
#endif

    int r = i2c_reg_read1(AXP173_BUS, AXP173_ADDR, 0x00);
    if(r < 0)
        return input_status;

    /* Check for AC input */
    if(r & 0x80)
        input_status |= AXP173_INPUT_AC;

    /* Only report USB if ACIN and VBUS are not shorted */
    if((r & 0x20) != 0 && (r & 0x02) == 0)
        input_status |= AXP173_INPUT_USB;

#ifdef HAVE_BATTERY_SWITCH
    /* Check for battery presence if target defines it as removable */
    r = i2c_reg_read1(AXP173_BUS, AXP173_ADDR, 0x01);
    if(r >= 0 && (r & 0x20) != 0)
        input_status |= AXP173_INPUT_BATTERY;
#endif

    return input_status;
}

int axp173_adc_read(int adc)
{
    int value = axp173_adc_read_raw(adc);
    if(value == INT_MIN)
        return INT_MIN;

    return axp173_adc_conv_raw(adc, value);
}

int axp173_adc_read_raw(int adc)
{
    /* Don't give a reading if the ADC is not enabled */
    if((axp173.adc_enable & (1 << adc)) == 0)
        return INT_MIN;

    /* Read the ADC */
    uint8_t buf[3];
    int count = (adc == ADC_BATTERY_POWER) ? 3 : 2;
    uint8_t reg = axp173_adc_info[adc].reg;
    int rc = i2c_reg_read(AXP173_BUS, AXP173_ADDR, reg, count, &buf[0]);
    if(rc != I2C_STATUS_OK)
        return INT_MIN;

    /* Parse the value */
    if(adc == ADC_BATTERY_POWER)
        return (buf[0] << 16) | (buf[1] << 8) | buf[2];
    else if(adc == ADC_CHARGE_CURRENT || adc == ADC_DISCHARGE_CURRENT)
        return (buf[0] << 5) | (buf[1] & 0x1f);
    else
        return (buf[0] << 4) | (buf[1] & 0xf);
}

int axp173_adc_conv_raw(int adc, int value)
{
    switch(adc) {
    case ADC_ACIN_VOLTAGE:
    case ADC_VBUS_VOLTAGE:
        /* 0 mV ... 6.9615 mV, step 1.7 mV */
        return value * 17 / 10;
    case ADC_ACIN_CURRENT:
        /* 0 mA ... 2.5594 A, step 0.625 mA */
        return value * 5 / 8;
    case ADC_VBUS_CURRENT:
        /* 0 mA ... 1.5356 A, step 0.375 mA */
        return value * 3 / 8;
    case ADC_INTERNAL_TEMP:
        /* -144.7 C ... 264.8 C, step 0.1 C */
        return value - 1447;
    case ADC_TS_INPUT:
        /* 0 mV ... 3.276 V, step 0.8 mV */
        return value * 4 / 5;
    case ADC_BATTERY_VOLTAGE:
        /* 0 mV ... 4.5045 V, step 1.1 mV */
        return value * 11 / 10;
    case ADC_CHARGE_CURRENT:
    case ADC_DISCHARGE_CURRENT:
        /* 0 mA to 4.095 A, step 0.5 mA */
        return value / 2;
    case ADC_APS_VOLTAGE:
        /* 0 mV to 5.733 V, step 1.4 mV */
        return value * 7 / 5;
    case ADC_BATTERY_POWER:
        /* 0 uW to 23.6404 W, step 0.55 uW */
        return value * 11 / 20;
    default:
        /* Shouldn't happen */
        return INT_MIN;
    }
}

int axp173_adc_get_enabled(void)
{
    return axp173.adc_enable;
}

void axp173_adc_set_enabled(int adc_bits)
{
    /* Ignore no-op */
    if(adc_bits == axp173.adc_enable)
        return;

    /* Compute the new register values */
    const struct axp173_adc_info* info = axp173_adc_info;
    uint8_t regs[2] = {0, 0};
    for(int i = 0; i < NUM_ADC_CHANNELS; ++i) {
        if(info[i].en_reg == 0xff)
            continue;

        if(adc_bits & (1 << i))
            regs[info[i].en_reg - 0x82] |= 1 << info[i].en_bit;
    }

    /* These ADCs share an enable bit */
    if(adc_bits & ((1 << ADC_CHARGE_CURRENT)|(1 << ADC_DISCHARGE_CURRENT))) {
        adc_bits |= (1 << ADC_CHARGE_CURRENT);
        adc_bits |= (1 << ADC_DISCHARGE_CURRENT);
    }

    /* Enable required bits for battery power ADC */
    if(adc_bits & (1 << ADC_BATTERY_POWER)) {
        regs[0] |= 1 << info[ADC_DISCHARGE_CURRENT].en_bit;
        regs[0] |= 1 << info[ADC_BATTERY_VOLTAGE].en_bit;
    }

    /* Update the configuration */
    i2c_reg_write(AXP173_BUS, AXP173_ADDR, 0x82, 2, &regs[0]);
    axp173.adc_enable = adc_bits;
}

int axp173_adc_get_rate(void)
{
    int r = i2c_reg_read1(AXP173_BUS, AXP173_ADDR, 0x84);
    if(r < 0)
        return AXP173_ADC_RATE_100HZ; /* an arbitrary value */

    return (r >> 6) & 3;
}

void axp173_adc_set_rate(int rate)
{
    i2c_reg_modify1(AXP173_BUS, AXP173_ADDR, 0x84,
                    0xc0, (rate & 3) << 6, NULL);
}

static uint32_t axp173_cc_parse(const uint8_t* buf)
{
    return ((uint32_t)buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

void axp173_cc_read(uint32_t* charge, uint32_t* discharge)
{
    uint8_t buf[8];
    int rc = i2c_reg_read(AXP173_BUS, AXP173_ADDR, 0xb0, 8, &buf[0]);
    if(rc != I2C_STATUS_OK) {
        if(charge)
            *charge = 0;
        if(discharge)
            *discharge = 0;
        return;
    }

    if(charge)
        *charge = axp173_cc_parse(&buf[0]);
    if(discharge)
        *discharge = axp173_cc_parse(&buf[4]);
}

void axp173_cc_clear(void)
{
    i2c_reg_setbit1(AXP173_BUS, AXP173_ADDR, 0xb8, 5, 1, NULL);
}

void axp173_cc_enable(bool en)
{
    i2c_reg_setbit1(AXP173_BUS, AXP173_ADDR, 0xb8, 7, en ? 1 : 0, NULL);
}

#ifndef BOOTLOADER
#define AXP173_DEBUG_BATTERY_STATUS 0
#define AXP173_DEBUG_INPUT_STATUS   1
#define AXP173_DEBUG_ADC_RATE       2
#define AXP173_DEBUG_FIRST_ADC      3
#define AXP173_DEBUG_ENTRIES        (AXP173_DEBUG_FIRST_ADC + NUM_ADC_CHANNELS)

static int axp173_debug_menu_cb(int action, struct gui_synclist* lists)
{
    (void)lists;

    if(action == ACTION_NONE)
        action = ACTION_REDRAW;

    return action;
}

static const char* axp173_debug_menu_get_name(int item, void* data,
                                              char* buf, size_t buflen)
{
    (void)data;

    static const char* const adc_names[] = {
        "V_acin", "I_acin", "V_vbus", "I_vbus", "T_int",
        "V_ts", "V_batt", "I_chrg", "I_dchg", "V_aps", "P_batt"
    };

    static const char* const adc_units[] = {
        "mV", "mA", "mV", "mA", "C", "mV", "mV", "mA", "mA", "mV", "uW",
    };

    int adc = item - AXP173_DEBUG_FIRST_ADC;
    if(item >= AXP173_DEBUG_FIRST_ADC && adc < NUM_ADC_CHANNELS) {
        int raw_value = axp173_adc_read_raw(adc);
        if(raw_value == INT_MIN) {
            snprintf(buf, buflen, "%s: [Disabled]", adc_names[adc]);
            return buf;
        }

        int value = axp173_adc_conv_raw(adc, raw_value);
        if(adc == ADC_INTERNAL_TEMP) {
            snprintf(buf, buflen, "%s: %d.%d %s", adc_names[adc],
                     value/10, value%10, adc_units[adc]);
        } else {
            snprintf(buf, buflen, "%s: %d %s", adc_names[adc],
                     value, adc_units[adc]);
        }

        return buf;
    }

    switch(item) {
    case AXP173_DEBUG_BATTERY_STATUS: {
        switch(axp173_battery_status()) {
        case AXP173_BATT_FULL:
            return "Battery: Full";
        case AXP173_BATT_CHARGING:
            return "Battery: Charging";
        case AXP173_BATT_DISCHARGING:
            return "Battery: Discharging";
        default:
            return "Battery: Unknown";
        }
    } break;

    case AXP173_DEBUG_INPUT_STATUS: {
        int s = axp173_input_status();
        const char* ac = (s & AXP173_INPUT_AC) ? " AC" : "";
        const char* usb = (s & AXP173_INPUT_USB) ? " USB" : "";
        const char* batt = (s & AXP173_INPUT_BATTERY) ? " Battery" : "";
        snprintf(buf, buflen, "Inputs:%s%s%s", ac, usb, batt);
        return buf;
    } break;

    case AXP173_DEBUG_ADC_RATE: {
        int rate = 25 << axp173_adc_get_rate();
        snprintf(buf, buflen, "ADC sample rate: %d Hz", rate);
        return buf;
    } break;

    default:
        return "---";
    }
}

bool axp173_debug_menu(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "AXP173 debug", AXP173_DEBUG_ENTRIES, NULL);
    info.action_callback = axp173_debug_menu_cb;
    info.get_name = axp173_debug_menu_get_name;
    return simplelist_show_list(&info);
}
#endif /* !BOOTLOADER */

/* This is basically the only valid implementation, so define it here */
unsigned int power_input_status(void)
{
    unsigned int state = 0;
    int input_status = axp173_input_status();

    if(input_status & AXP173_INPUT_AC)
        state |= POWER_INPUT_MAIN_CHARGER;

    if(input_status & AXP173_INPUT_USB)
        state |= POWER_INPUT_USB_CHARGER;

#ifdef HAVE_BATTERY_SWITCH
    if(input_status & AXP173_INPUT_BATTERY)
        state |= POWER_INPUT_BATTERY;
#endif

    return state;
}
