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

#ifndef __AXP192_H__
#define __AXP192_H__

#include <stdint.h>
#include <stdbool.h>

enum {
#define DEFREG(regname, addr) AXP_REG_##regname = addr,
#include "axp192-defs.h"
};

enum {
#define DEFFLD(regname, fldname, msb, lsb, ...) \
    BM_AXP_##regname##_##fldname = ((1 << ((msb) - (lsb) + 1)) - 1) << lsb, \
    BP_AXP_##regname##_##fldname = lsb,
#include "axp192-defs.h"
};

enum {
    AXP_SUPPLY_EXTEN,
    AXP_SUPPLY_DCDC1,
    AXP_SUPPLY_DCDC2,
    AXP_SUPPLY_DCDC3,
    AXP_SUPPLY_LDO2,
    AXP_SUPPLY_LDO3,
    AXP_SUPPLY_LDOIO0,
    AXP_NUM_SUPPLIES,
};

enum {
    AXP_ADC_ACIN_VOLTAGE,
    AXP_ADC_ACIN_CURRENT,
    AXP_ADC_VBUS_VOLTAGE,
    AXP_ADC_VBUS_CURRENT,
    AXP_ADC_INTERNAL_TEMP,
    AXP_ADC_TS_INPUT,
    AXP_ADC_GPIO0,
    AXP_ADC_GPIO1,
    AXP_ADC_GPIO2,
    AXP_ADC_GPIO3,
    AXP_ADC_BATTERY_VOLTAGE,
    AXP_ADC_CHARGE_CURRENT,
    AXP_ADC_DISCHARGE_CURRENT,
    AXP_ADC_APS_VOLTAGE,
    AXP_NUM_ADCS,
};

enum {
    AXP_GPIO_OPEN_DRAIN_OUTPUT = 0x0,
    AXP_GPIO_INPUT             = 0x1,
    AXP_GPIO_SPECIAL           = 0x2,
    AXP_GPIO_ADC_IN            = 0x4,
    AXP_GPIO_LOW_OUTPUT        = 0x5,
    AXP_GPIO_FLOATING          = 0x7,
};

enum {
    /* Limit USB current consumption to 100 mA. */
    AXP_VBUS_LIMIT_100mA = (1 << BP_AXP_VBUSIPSOUT_VHOLD_LIM) |
                           (1 << BP_AXP_VBUSIPSOUT_VBUS_LIM) |
                           (1 << BP_AXP_VBUSIPSOUT_LIM_100mA),

    /* Limit USB current consumption to 500 mA. */
    AXP_VBUS_LIMIT_500mA = (1 << BP_AXP_VBUSIPSOUT_VHOLD_LIM) |
                           (1 << BP_AXP_VBUSIPSOUT_VBUS_LIM) |
                           (0 << BP_AXP_VBUSIPSOUT_LIM_100mA),

    /* No upper bound on USB current, but the current will still
     * be reduced to maintain the bus voltage above V_hold. */
    AXP_VBUS_UNLIMITED = (1 << BP_AXP_VBUSIPSOUT_VHOLD_LIM) |
                         (0 << BP_AXP_VBUSIPSOUT_VBUS_LIM) |
                         (0 << BP_AXP_VBUSIPSOUT_LIM_100mA),

    /* Unlimited USB current consumption. Voltage is allowed to drop
     * below V_hold, which may interfere with normal USB operation.
     * This mode is really only useful with AC charging adapters. */
    AXP_VBUS_FULLY_UNLIMITED = (0 << BP_AXP_VBUSIPSOUT_VHOLD_LIM) |
                               (0 << BP_AXP_VBUSIPSOUT_VBUS_LIM) |
                               (0 << BP_AXP_VBUSIPSOUT_LIM_100mA),
};

extern int axp_read(uint8_t reg);
extern int axp_write(uint8_t reg, uint8_t value);
extern int axp_modify(uint8_t reg, uint8_t clr, uint8_t set);

extern void axp_enable_supply(int supply, bool enable);
extern void axp_set_enabled_supplies(unsigned int supply_mask);
extern void axp_set_supply_voltage(int supply, int output_mV);

extern void axp_enable_adc(int adc, bool enable);
extern void axp_set_enabled_adcs(unsigned int adc_mask);
extern int axp_read_adc_raw(int adc);
extern int axp_conv_adc(int adc, int value);
extern int axp_read_adc(int adc);

extern void axp_set_gpio_function(int gpio, int function);
extern void axp_set_gpio_pulldown(int gpio, bool enable);
extern int axp_get_gpio(int gpio);
extern void axp_set_gpio(int gpio, bool enable);

extern void axp_set_charge_current(int current_mA);
extern int axp_get_charge_current(void);
extern void axp_set_vbus_limit(int vbus_limit);
extern void axp_set_vhold_level(int vhold_mV);
extern bool axp_is_charging(void);
extern unsigned int axp_power_input_status(void);

extern void axp_power_off(void);

#endif /* __AXP192_H__ */
