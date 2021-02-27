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

/* Read an ADC channel. Returns raw ADC value, or -1 on error.
 * All ADCs are 12 bits, except for ADC_BATTERY_POWER, which is 24 bits.
 */
extern int axp173_adc_read(int adc);

/* Convert a raw ADC reading into sensible units.
 *
 * - For voltages the output scale is mV.
 * - For currents the output scale is mA.
 * - For temperatures the output scale is 0.1 degrees Celsius.
 * - For power the output scale is uW.
 */
extern int axp173_adc_conv(int adc, int value);

/* Enable or disable an ADC.
 * Returns 0 on success, or negative value on error.
 *
 * - ADC_CHARGE_CURRENT and ADC_DISCHARGE_CURRENT share the same enable
 *   bit so any change to one will be reflected in the other.
 *
 * - ADC_BATTERY_POWER is a synthetic reading produced by multiplying
 *   the battery voltage and discharge current, so it cannot be enabled
 *   or disabled and will always return -1.
 */
extern int axp173_adc_set_enabled(int adc, int enabled);

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

#define axp173_reg_read(reg, val)  axp173_reg_clrset((reg), 0, 0)
#define axp173_reg_write(reg, val) axp173_reg_clrset((reg), 0xff, (val))
#define axp173_reg_set(reg, val)   axp173_reg_clrset((reg), 0, (val))
#define axp173_reg_clr(reg, val)   axp173_reg_clrset((reg), (val), 0)

#endif /* __AXP173_H__ */
