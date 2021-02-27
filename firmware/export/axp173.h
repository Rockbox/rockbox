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

#ifndef __AXP173_H__
#define __AXP173_H__

#include <stdbool.h>

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
#define ADC_BATTERY_POWER       10
#define NUM_ADC_CHANNELS        11

#define AXP173_CC_CHARGE    0xb0
#define AXP173_CC_DISCHARGE 0xb4

/* Target-specific, normally called from power_init() */
extern void axp173_init(void);

/* ADC channels are 12 bits except for ADC_BATTERY_POWER, which is 24 bits.
 * All ADC functions return a negative value on error.
 *
 * - axp173_adc_read() returns a normalized ADC reading
 * - axp173_adc_read_raw() returns a raw ADC reading
 * - axp173_adc_conv_raw() converts a raw ADC reading to a normalized one
 * - axp173_adc_set_enabled() enables or disables an ADC
 *
 * The scale of the normalized reading is determined as follows:
 *
 * - For voltages the output is in millivolts.
 * - For currents the output is in milliamps.
 * - For temperatures the output is in tenths of a degree Celsius.
 * - For power the output scale is microwatts.
 *
 * Note that not all ADCs can be enabled/disabled independently:
 *
 * - ADC_CHARGE_CURRENT and ADC_DISCHARGE_CURRENT share the same enable bit.
 * - ADC_BATTERY_POWER is a synthetic product of voltage & current so it
 *   cannot be independently controlled.
 */
extern int axp173_adc_read(int adc);
extern int axp173_adc_read_raw(int adc);
extern int axp173_adc_conv_raw(int adc, int value);
extern int axp173_adc_enable(int adc, bool enabled);

/* The AXP173 is equipped with a coulomb counter (aka "fuel gauge") allowing
 * it to provide accurate real-time information about the battery charge level.
 * All functions return a negative value on error.
 *
 * - axp173_cc_read() gets the value of a counter (CC_CHARGE or CC_DISCHARGE).
 * - axp173_cc_clear() resets the counters to 0.
 * - axp173_cc_pause() pauses the counters.
 * - axp173_cc_enable() enables/disables the counters.
 */
extern int axp173_cc_read(int counter, unsigned* value);
extern int axp173_cc_clear(void);
extern int axp173_cc_pause(bool paused);
extern int axp173_cc_enable(bool en);

/* TODO: support for IRQs */
extern void axp173_irq_enable(bool en);

/* Perform a clear-and-set operation on the contents of a register.
 * Returns the updated register value, or a negative number on error.
 *
 * (Target-specific function)
 */
extern int axp173_reg_clrset(int reg, int clr, int set);

/* Read multiple consective registers and store the values into a buffer.
 * Returns 0 on success, or negative value on error.
 *
 * (Target-specific function)
 */
extern int axp173_reg_read_multi(int reg, int count, unsigned char* buf);

#define axp173_reg_read(reg)       axp173_reg_clrset((reg), 0, 0)
#define axp173_reg_write(reg, val) axp173_reg_clrset((reg), 0xff, (val))
#define axp173_reg_set(reg, val)   axp173_reg_clrset((reg), 0, (val))
#define axp173_reg_clr(reg, val)   axp173_reg_clrset((reg), (val), 0)

#endif /* __AXP173_H__ */
