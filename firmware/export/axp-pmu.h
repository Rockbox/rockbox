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

#ifndef __AXP_PMU_H__
#define __AXP_PMU_H__

#include "config.h"
#include <stdbool.h>
#include <stdint.h>

/* ADC channels */
#define ADC_ACIN_VOLTAGE        0
#define ADC_ACIN_CURRENT        1
#define ADC_VBUS_VOLTAGE        2
#define ADC_VBUS_CURRENT        3
#define ADC_INTERNAL_TEMP       4
#define ADC_TS_INPUT            5
#define ADC_BATTERY_VOLTAGE     6
#define ADC_CHARGE_CURRENT      7
#define ADC_DISCHARGE_CURRENT   8
#define ADC_APS_VOLTAGE         9
#define NUM_ADC_CHANNELS        10

/* ADC sampling rates */
#define AXP_ADC_RATE_25HZ   0
#define AXP_ADC_RATE_50HZ   1
#define AXP_ADC_RATE_100HZ  2
#define AXP_ADC_RATE_200HZ  3

/* Return values of axp_battery_status() */
#define AXP_BATT_DISCHARGING    0
#define AXP_BATT_CHARGING       1
#define AXP_BATT_FULL           2

/* Bits returned by axp_input_status() */
#define AXP_INPUT_AC        (1 << 0)
#define AXP_INPUT_USB       (1 << 1)
#define AXP_INPUT_BATTERY   (1 << 2)
#define AXP_INPUT_EXTERNAL  (AXP_INPUT_AC|AXP_INPUT_USB)

/* Power supplies known by this driver. Not every chip has all supplies! */
#define AXP_SUPPLY_DCDC1    0
#define AXP_SUPPLY_DCDC2    1
#define AXP_SUPPLY_DCDC3    2
#define AXP_SUPPLY_LDO1     3
#define AXP_SUPPLY_LDO2     4
#define AXP_SUPPLY_LDO3     5
#define AXP_SUPPLY_LDO_IO0  6
#define AXP_NUM_SUPPLIES    7

/* Special values returned by axp_supply_get_voltage */
#define AXP_SUPPLY_NOT_PRESENT INT_MIN
#define AXP_SUPPLY_DISABLED    (-1)

/* Registers -- common to AXP173 and AXP192 (incomplete listing) */
#define AXP_REG_POWERSTATUS         0x00
#define AXP_REG_CHARGESTATUS        0x01
#define AXP_REG_CHIP_ID             0x03
#define AXP_REG_PWROUTPUTCTRL1      0x10
#define AXP_REG_PWROUTPUTCTRL2      0x12
#define AXP_REG_SHUTDOWNLEDCTRL     0x32
#define AXP_REG_CHARGECONTROL1      0x33
#define AXP_REG_DCDCWORKINGMODE     0x80
#define AXP_REG_ADCENABLE1          0x82
#define AXP_REG_ADCENABLE2          0x83
#define AXP_REG_ADCSAMPLERATE       0x84
#define AXP_REG_COULOMBCOUNTERBASE  0xb0
#define AXP_REG_COULOMBCOUNTERCTRL  0xb8

/* AXP192-only registers (incomplete listing) */
#define AXP192_REG_GPIO0FUNCTION    0x90
#define AXP192_REG_GPIO1FUNCTION    0x92
#define AXP192_REG_GPIO2FUNCTION    0x93
#define AXP192_REG_GPIOSTATE1       0x94

/* Must be called from power_init() to initialize the driver state */
extern void axp_init(void);

/* - axp_supply_set_voltage(): set a supply voltage to the given value
 *   in millivolts. Pass a voltage of AXP_SUPPLY_DISABLED to shut off
 *   the supply. Any invalid supply or voltage will make the call a no-op.
 *
 * - axp_supply_get_voltage() returns a supply voltage in millivolts.
 *   If the supply is powered off, returns AXP_SUPPLY_DISABLED.
 *   If the chip does not have the supply, returns AXP_SUPPLY_NOT_PRESENT.
 */
extern void axp_supply_set_voltage(int supply, int voltage);
extern int axp_supply_get_voltage(int supply);

/* Basic battery and power supply status */
extern int axp_battery_status(void);
extern int axp_input_status(void);

/* ADC access -- ADCs which are not enabled will return INT_MIN if read.
 * The output of axp_adc_read() is normalized to appropriate units:
 *
 * - for voltages, the scale is millivolts
 * - for currents, the scale is milliamps
 * - for temperatures, the scale is tenths of a degree Celsius
 * - for power, the scale is microwatts
 *
 * See the comment in axp_adc_conv_raw() for raw value precision/scale.
 */
extern int axp_adc_read(int adc);
extern int axp_adc_read_raw(int adc);
extern int axp_adc_conv_raw(int adc, int value);
extern void axp_adc_set_enabled(int adc_bits);
extern int axp_adc_get_rate(void);
extern void axp_adc_set_rate(int rate);

/* - axp_cc_read() reads the coulomb counters
 * - axp_cc_clear() resets both counters to zero
 * - axp_cc_enable() will stop/start the counters running
 * - axp_cc_is_enabled() returns true if the counters are running
 */
extern void axp_cc_read(uint32_t* charge, uint32_t* discharge);
extern void axp_cc_clear(void);
extern void axp_cc_enable(bool en);
extern bool axp_cc_is_enabled(void);

/* Set/get maximum charging current in milliamps */
extern void axp_set_charge_current(int current_mA);
extern int axp_get_charge_current(void);

/* Set the shutdown bit */
extern void axp_power_off(void);

/* Debug menu */
extern bool axp_debug_menu(void);

#endif /* __AXP_PMU_H__ */
