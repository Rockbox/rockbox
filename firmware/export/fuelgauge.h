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

#ifndef __FUELGAUGE_H__
#define __FUELGAUGE_H__

/* Generic fuel gauge driver
 * -------------------------
 *
 * This driver is a frontend designed to take advantage of the features found
 * in modern power management ICs to provide more accurate battery estimates
 * than voltage can do on its own.
 *
 * Features:
 *
 * - Smoothing voltage / current / power consumption ADCs
 * - Time remaining calculation based on measured current/energy usage
 * - State-of-charge (SoC) estimation and reporting
 * - State-of-energy (SoE) reporting based on voltage-SoC curve
 * - State-of-health (SoH) report
 *
 * Targets must define HAVE_FUELGAUGE and CONFIG_CHARGING = CHARGING_TARGET
 * to use this driver. Measurements and events are supplied by the target by
 * implementing the backend API functions.
 */

#include <stdint.h>
#include <stdbool.h>

/* Target capabilities: the target should define FUELGAUGE_CAPS to indicate
 * what kind of data can be reported to the fuel gauge driver. To get any
 * real benefit from fuelgauge, the target needs voltage and current ADCs
 * and a hardware coulomb counter, so these capabilities don't have bits.
 *
 * - FUELGAUGE_CAP_POWER
 *      Target can measure instantaneous power flow into/out of the battery.
 *      Emulated in software if not available.
 *
 * - FUELGAUGE_CAP_TEMPERATURE
 *      Target can measure the battery temperature, or at least something
 *      that approximates the battery temperature fairly well. Without a
 *      hardware reading we just use a constant "reasonable" temperature.
 */
#define FUELGAUGE_CAP_POWER         (1 << 0)
#define FUELGAUGE_CAP_TEMPERATURE   (1 << 1)

/* Limits of raw ADC readings */
#define FUELGAUGE_ADC_SAMPLES   128
#define FUELGAUGE_ADC_MAX       (LONG_MAX / FUELGAUGE_ADC_SAMPLES)

/* Battery status values */
#define FUELGAUGE_BATTERY_DISCHARGING   0
#define FUELGAUGE_BATTERY_CHARGING      1
#define FUELGAUGE_BATTERY_FULL          2

/* Bits corresponding to fields in the fuelgauge_data struct */
#define FUELGAUGE_DATA_CHARGE_LIMITS    (1 << 0)
#define FUELGAUGE_DATA_ALL              0xff

/* Fuel gauge system states */
#define FUELGAUGE_STATE_NORMAL  0
#define FUELGAUGE_STATE_FULL    1
#define FUELGAUGE_STATE_EMPTY   2

#include "fuelgauge-target.h"

typedef struct fuelgauge_data {
    /* Coulomb counter min/max values. Used to infer the battery capacity
     * and charge level from the coulomb counter; it's basically required
     * to persist these when using coulomb counting.
     *
     * Bit: FUELGAUGE_DATA_CHARGE_LIMITS
     */
    long charge_min;
    long charge_max;
} fuelgauge_data;

/* Reset fuel gauge data */
extern void fuelgauge_reset(void);

/* Read current fuel gauge state */
extern int fuelgauge_state(void);

/* SoC, SoE, and SoH readings */
extern long fuelgauge_vsoc(void);
extern long fuelgauge_isoc(void);
extern long fuelgauge_soe(void);
extern long fuelgauge_soh(void);

/* Raw charge/current data */
extern long fuelgauge_charge_now(void);
extern long fuelgauge_charge_full(void);
extern long fuelgauge_current_now(void);

/* Raw energy/power data */
extern long fuelgauge_energy_now(void);
extern long fuelgauge_energy_full(void);
extern long fuelgauge_power_now(void);

/* Debug menu */
extern bool fuelgauge_debug_menu(void);

/* Since the usual CHARGING_TARGET entry points are taken up by the fuel gauge
 * driver, these functions let the target hook into them.
 *
 * The init and step functions are called first thing, before any generic code
 * runs, and the shutdown function is called after the generic code finishes
 * its own shutdown.
 */
extern void fuelgauge_tgt_init(void);
extern void fuelgauge_tgt_step(void);
extern void fuelgauge_tgt_shutdown(void);

/* Return the status of the battery: either discharging, charging, or full. */
extern int fuelgauge_tgt_battery_status(void);

/* Read raw ADC measurements. These are filtered by averaging a number of
 * samples over time, so the return value must be less than FUELGAUGE_ADC_MAX,
 * which is a range of 24 bits assuming a 32-bit long.
 *
 * ADC readings have to be linear, ie. the change in physical quantity
 * between steps x and x+1 must not depend on x. If this is not the case,
 * the scale must be linearized before returning the reading.
 *
 * ADC readings cannot be negative, so current and power must not change sign
 * based on the direction of charge/discharge. Negative values are assumed to
 * be erroneous and ignored; useful if your ADCs need time to get an accurate
 * reading after a change of power inputs.
 */
extern long fuelgauge_tgt_read_voltage(void);
extern long fuelgauge_tgt_read_current(void);
extern long fuelgauge_tgt_read_power(void);
extern long fuelgauge_tgt_read_temperature(void);

/* Read/clear the hardware coulomb counters. These are not filtered since
 * errors in the coulomb counters are cumulative rather than random. */
extern long fuelgauge_tgt_read_charge(void);
extern void fuelgauge_tgt_clear_charge(void);

/* Convert and round raw ADC measurements to physical units.
 *
 * - voltage should be converted to millivolts (mV)
 * - current should be converted to milliamps (mA)
 * - power should be converted to milliwatts (mW)
 * - temperature should be converted to units of 0.1 degrees C
 * - charge should be converted to milliamp-hours (mAh)
 */
extern long fuelgauge_tgt_convert_voltage(long value);
extern long fuelgauge_tgt_convert_current(long value);
extern long fuelgauge_tgt_convert_power(long value);
extern long fuelgauge_tgt_convert_temperature(long value);
extern long fuelgauge_tgt_convert_charge(long value);

/* Save and load the fuelgauge data to non-volatile memory. This must survive
 * a power cycle to be of any use, but it doesn't need to survive a total loss
 * of power.
 *
 * For saving, a dirty mask is passed in indicating which fields have changed
 * since the last save/load.
 *
 * For loading, return a mask of the fields you loaded successfully. You must
 * only change the value of fields if you actually load them successfully;
 * other fields must be left alone.
 *
 * Targets should try to detect corrupted fields and not load them, so the
 * driver can load sane defaults instead of having to correct wacky values.
 */
extern void fuelgauge_tgt_save_data(unsigned int bits,
                                    const fuelgauge_data* fgd);
extern unsigned int fuelgauge_tgt_load_data(fuelgauge_data* fgd);

#endif /* __FUELGAUGE_H__ */
