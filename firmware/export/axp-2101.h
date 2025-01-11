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

#ifndef __AXP_2101_H__
#define __AXP_2101_H__

#include "config.h"
#include <stdbool.h>
#include <stdint.h>

/* ADC channels */
#define AXP2101_ADC_VBAT_VOLTAGE    0
#define AXP2101_ADC_VBUS_VOLTAGE    1
#define AXP2101_ADC_VSYS_VOLTAGE    2
#define AXP2101_ADC_TS_VOLTAGE      3
#define AXP2101_ADC_DIE_TEMPERATURE 4
#define AXP2101_NUM_ADC_CHANNELS    5

/* ADC sampling rates */
#define AXP2101_ADC_RATE_25HZ   0
#define AXP2101_ADC_RATE_50HZ   1
#define AXP2101_ADC_RATE_100HZ  2
#define AXP2101_ADC_RATE_200HZ  3

/* Return values of axp_battery_status() */
#define AXP2101_BATT_DISCHARGING    0
#define AXP2101_BATT_CHARGING       1
#define AXP2101_BATT_FULL           2

/* Bits returned by axp_input_status() */
#define AXP2101_INPUT_AC        (1 << 0)
#define AXP2101_INPUT_USB       (1 << 1)
#define AXP2101_INPUT_BATTERY   (1 << 2)
#define AXP2101_INPUT_EXTERNAL  (AXP_INPUT_AC|AXP_INPUT_USB)

#define AXP2101_SUPPLY_DCDC1    0
#define AXP2101_SUPPLY_DCDC2    1
#define AXP2101_SUPPLY_DCDC3    2
#define AXP2101_SUPPLY_DCDC4    3
#define AXP2101_SUPPLY_DCDC5    4
#define AXP2101_SUPPLY_ALDO1    5
#define AXP2101_SUPPLY_ALDO2    6
#define AXP2101_SUPPLY_ALDO3    7
#define AXP2101_SUPPLY_ALDO4    8
#define AXP2101_SUPPLY_BLDO1    9
#define AXP2101_SUPPLY_BLDO2    10
#define AXP2101_SUPPLY_DLDO1    11
#define AXP2101_SUPPLY_DLDO2    12
#define AXP2101_SUPPLY_VCPUS    13
#define AXP2101_SUPPLY_RTCLDO1  14
#define AXP2101_SUPPLY_RTCLDO2  15
#define AXP2101_NUM_SUPPLIES    16

/* Special values returned by axp_supply_get_voltage */
#define AXP2101_SUPPLY_NOT_PRESENT INT_MIN
#define AXP2101_SUPPLY_DISABLED    (-1)

/* AXP2101 registers */
#define AXP2101_REG_PMU_STATUS1     0x00
#define AXP2101_REG_PMU_STATUS2     0x01
#define AXP2101_REG_DATA_BUFFER0    0x04
#define AXP2101_REG_DATA_BUFFER1    0x05
#define AXP2101_REG_DATA_BUFFER2    0x06
#define AXP2101_REG_DATA_BUFFER3    0x07
#define AXP2101_REG_PMUCOMMCONFIG   0x10
#define AXP2101_REG_BATFET_CTRL     0x12
#define AXP2101_REG_DIETEMPCTRL     0x13
#define AXP2101_REG_MINSYSVOLTCTRL  0x14
#define AXP2101_REG_INVOLTLIMITCTRL 0x15
#define AXP2101_REG_INCURRLIMITCTRL 0x16
#define AXP2101_REG_FUELGAUGERESET  0x17
#define AXP2101_REG_PERIPHERALCTRL  0x18
#define AXP2101_REG_WATCHDOGCTRL    0x19
#define AXP2101_REG_LOWBATTWARN     0x1a
#define AXP2101_REG_PWRONSTATUS     0x20
#define AXP2101_REG_PWROFFSTATUS    0x21
#define AXP2101_REG_PWROFFENABLE    0x22
#define AXP2101_REG_PWROFF_DCDC_OVP_UVP 0x23
#define AXP2101_REG_VSYSPWROFFTHRESH 0x24
#define AXP2101_REG_PWROK_SETTING   0x25
#define AXP2101_REG_SLEEPWAKE       0x26
#define AXP2101_REG_LOGICTHRESH     0x27
#define AXP2101_REG_FASTPWRON0      0x28
#define AXP2101_REG_FASTPWRON1      0x29
#define AXP2101_REG_FASTPWRON2      0x2a
#define AXP2101_REG_FASTPWRON_CTRL  0x2b
#define AXP2101_REG_ADCCHNENABLE    0x30
#define AXP2101_REG_ADC_VBAT_H      0x34
#define AXP2101_REG_ADC_VBAT_L      0x35
#define AXP2101_REG_ADC_TS_H        0x36
#define AXP2101_REG_ADC_TS_L        0x37
#define AXP2101_REG_ADC_VBUS_H      0x38
#define AXP2101_REG_ADC_VBUS_L      0x39
#define AXP2101_REG_ADC_VSYS_H      0x3a
#define AXP2101_REG_ADC_VSYS_L      0x3b
#define AXP2101_REG_ADC_TDIE_H      0x3c
#define AXP2101_REG_ADC_TDIE_L      0x3d
#define AXP2101_REG_IRQEN0          0x40
#define AXP2101_REG_IRQEN1          0x41
#define AXP2101_REG_IRQEN2          0x42
#define AXP2101_REG_IRQSTATUS0      0x48
#define AXP2101_REG_IRQSTATUS1      0x49
#define AXP2101_REG_IRQSTATUS2      0x4a
#define AXP2101_REG_TSPINCTRL       0x50
#define AXP2101_REG_TS_HYSL2H       0x52
#define AXP2101_REG_TS_HYSH2L       0x53
#define AXP2101_REG_VLTF_CHG        0x54
#define AXP2101_REG_VHTF_CHG        0x55
#define AXP2101_REG_VLTF_WORK       0x56
#define AXP2101_REG_VHTF_WORK       0x57
#define AXP2101_REG_JIETA_ENABLE    0x58
#define AXP2101_REG_JIETA_SETTING0  0x59
#define AXP2101_REG_JIETA_SETTING1  0x5a
#define AXP2101_REG_JIETA_SETTING2  0x5b
#define AXP2101_REG_IPRECHG_SETTING 0x61
#define AXP2101_REG_ICC_SETTING     0x62
#define AXP2101_REG_ITERM_SETTING   0x63
#define AXP2101_REG_CV_SETTING      0x64
#define AXP2101_REG_THERMREGTHRESH  0x65
#define AXP2101_REG_CHARGETIMEOUT   0x67
#define AXP2101_REG_BATTDETECTCTRL  0x68
#define AXP2101_REG_CHGLED          0x69
#define AXP2101_REG_BUTTONBAT       0x6a
#define AXP2101_REG_DCDC_ONOFF     0x80
#define AXP2101_REG_DCDC_FORCEPWM  0x81
#define AXP2101_REG_DCDC0_VOLTAGE   0x82
#define AXP2101_REG_DCDC1_VOLTAGE   0x83
#define AXP2101_REG_DCDC2_VOLTAGE   0x84
#define AXP2101_REG_DCDC3_VOLTAGE   0x85
#define AXP2101_REG_DCDC4_VOLTAGE   0x86
#define AXP2101_REG_LDO_ONOFF0      0x90
#define AXP2101_REG_LDO_ONOFF1      0x91
#define AXP2101_REG_LDO0_VOLTAGE    0x92
#define AXP2101_REG_LDO1_VOLTAGE    0x93
#define AXP2101_REG_LDO2_VOLTAGE    0x94
#define AXP2101_REG_LDO3_VOLTAGE    0x94
#define AXP2101_REG_LDO4_VOLTAGE    0x95
#define AXP2101_REG_LDO5_VOLTAGE    0x96
#define AXP2101_REG_LDO6_VOLTAGE    0x97
#define AXP2101_REG_LDO7_VOLTAGE    0x98
#define AXP2101_REG_LDO8_VOLTAGE    0x99
#define AXP2101_REG_LDO9_VOLTAGE    0x9a
#define AXP2101_REG_BATT_PARAMETER  0xa1
#define AXP2101_REG_FUEL_GAUGE_CTRL 0xa2
#define AXP2101_REG_BATT_PERCENTAGE 0xa4

/* Must be called from power_init() to initialize the driver state */
extern void axp2101_init(void);

/* - axp_supply_set_voltage(): set a supply voltage to the given value
 *   in millivolts. Pass a voltage of AXP_SUPPLY_DISABLED to shut off
 *   the supply. Any invalid supply or voltage will make the call a no-op.
 *
 * - axp_supply_get_voltage() returns a supply voltage in millivolts.
 *   If the supply is powered off, returns AXP_SUPPLY_DISABLED.
 *   If the chip does not have the supply, returns AXP_SUPPLY_NOT_PRESENT.
 */
extern void axp2101_supply_set_voltage(int supply, int voltage);
extern int axp2101_supply_get_voltage(int supply);

/* Basic battery and power supply status */
extern int axp2101_battery_status(void);
extern int axp2101_input_status(void);

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
extern int axp2101_adc_read(int adc);
extern int axp2101_adc_read_raw(int adc);
extern int axp2101_adc_conv_raw(int adc, int value);
extern void axp2101_adc_set_enabled(int adc_bits);
extern int axp2101_adc_get_rate(void);

extern int axp2101_egauge_read(void);

/* Set/get maximum charging current in milliamps */
extern void axp2101_set_charge_current(int current_mA);
extern int axp2101_get_charge_current(void);

/* Set the shutdown bit */
extern void axp2101_power_off(void);

/* Debug menu */
extern bool axp2101_debug_menu(void);

extern unsigned int axp2101_power_input_status(void);

#endif /* __AXP_2101_H__ */
