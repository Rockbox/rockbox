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
#include <stdint.h>

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

/* ADC sampling rates */
#define AXP173_ADC_RATE_25HZ    0
#define AXP173_ADC_RATE_50HZ    1
#define AXP173_ADC_RATE_100HZ   2
#define AXP173_ADC_RATE_200HZ   3

/* Return values of axp173_battery_status() */
#define AXP173_BATT_DISCHARGING   0
#define AXP173_BATT_CHARGING      1
#define AXP173_BATT_FULL          2

/* Bits returned by axp173_input_status() */
#define AXP173_INPUT_AC         (1 << 0)
#define AXP173_INPUT_USB        (1 << 1)
#define AXP173_INPUT_BATTERY    (1 << 2)
#define AXP173_INPUT_EXTERNAL   (AXP173_INPUT_AC|AXP173_INPUT_USB)

/* Must be called from power_init() to initialize the driver state */
extern void axp173_init(void);

/* Basic battery and power supply status */
extern int axp173_battery_status(void);
extern int axp173_input_status(void);

/* ADC access -- ADCs which are not enabled will return INT_MIN if read.
 * The output of axp173_adc_read() is normalized to appropriate units:
 *
 * - for voltages, the scale is millivolts
 * - for currents, the scale is milliamps
 * - for temperatures, the scale is tenths of a degree Celsius
 * - for power, the scale is microwatts
 *
 * See the comment in axp173_adc_conv_raw() for raw value precision/scale.
 */
extern int axp173_adc_read(int adc);
extern int axp173_adc_read_raw(int adc);
extern int axp173_adc_conv_raw(int adc, int value);
extern int axp173_adc_get_enabled(void);
extern void axp173_adc_set_enabled(int adc_bits);
extern int axp173_adc_get_rate(void);
extern void axp173_adc_set_rate(int rate);

/* - axp173_cc_read() reads the coulomb counters
 * - axp173_cc_clear() resets both counters to zero
 * - axp173_cc_enable() will stop/start the counters running
 */
extern void axp173_cc_read(uint32_t* charge, uint32_t* discharge);
extern void axp173_cc_clear(void);
extern void axp173_cc_enable(bool en);

/* Debug menu */
extern bool axp173_debug_menu(void);

#endif /* __AXP173_H__ */
