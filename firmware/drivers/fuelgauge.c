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

#include "fuelgauge.h"
#include "powermgmt.h"
#include "system.h"
#include "panic.h"
#include "logf.h"

/* For the debug menu */
#ifndef BOOTLOADER
# include "action.h"
# include "list.h"
# include <stdio.h>
#endif

/* must not equal any "real" battery status */
#define FUELGAUGE_BATTERY_UNKNOWN (-1)

static struct fuelgauge_state {
    /* Battery status and latched ADC battery status so we can detect
     * changes and reset ADC filters when they happen */
    int cur_battery_status;
    int last_adc_battery_status;

    /* Sums/last valid readings, for filtering raw ADC values */
    long v_batt_sum, i_batt_sum, p_batt_sum, temp_sum;
    long v_batt_last, i_batt_last, p_batt_last, temp_last;

    /* Filtered and converted ADC values for general use */
    long v_batt, i_batt, p_batt, temp;

    /* Latest coulomb counter sample */
    long charge;

    /* Indicates which parts of "fgd" need saving */
    unsigned int dirty;

    /* Current system state */
    int state;
} fgs;

static fuelgauge_data fgd;

static void fuelgauge_step_adc(void)
{
    /* save typing */
    const long NSAMP = FUELGAUGE_ADC_SAMPLES;

    /* Sample the ADCs */
    long vbr = fuelgauge_tgt_read_voltage();
    long ibr = fuelgauge_tgt_read_current();
    long pbr = fuelgauge_tgt_read_power();
    long tr = fuelgauge_tgt_read_temperature();

    /* Use the last valid value if a reading cannot be made */
    long vb = vbr, ib = ibr, pb = pbr, t = tr;
    if(vbr < 0) vb = fgs.v_batt_last; else fgs.v_batt_last = vbr;
    if(ibr < 0) ib = fgs.i_batt_last; else fgs.i_batt_last = ibr;
    if(pbr < 0) pb = fgs.p_batt_last; else fgs.p_batt_last = pbr;
    if(tr < 0)  t  = fgs.temp_last;   else fgs.temp_last   = tr;

    if(fgs.cur_battery_status == fgs.last_adc_battery_status) {
        /* Update the cumulative sum */
        fgs.v_batt_sum += vb - fgs.v_batt_sum / NSAMP;
        fgs.i_batt_sum += ib - fgs.i_batt_sum / NSAMP;
        fgs.p_batt_sum += pb - fgs.p_batt_sum / NSAMP;
    } else {
        /* Battery status change requires flushing the filter */
        fgs.v_batt_sum = vb * NSAMP;
        fgs.i_batt_sum = ib * NSAMP;
        fgs.p_batt_sum = pb * NSAMP;

        /* Don't resume normal operation until all readings go valid */
        if(vbr > 0 && ibr > 0 && pbr > 0)
            fgs.last_adc_battery_status = fgs.cur_battery_status;
    }

    /* Temperature doesn't change abruptly due to battery events,
     * hence the filter does not need to be flushed */
    fgs.temp_sum += t - fgs.temp_sum / NSAMP;

    /* Convert the smoothed readings to physical units */
    fgs.v_batt = fuelgauge_tgt_convert_voltage(fgs.v_batt_sum / NSAMP);
    fgs.i_batt = fuelgauge_tgt_convert_current(fgs.i_batt_sum / NSAMP);
    fgs.p_batt = fuelgauge_tgt_convert_power(fgs.p_batt_sum / NSAMP);
    fgs.temp = fuelgauge_tgt_convert_temperature(fgs.temp_sum / NSAMP);
}

void powermgmt_init_target(void)
{
    fuelgauge_tgt_init();

    /* Read initial battery status */
    fgs.cur_battery_status = fuelgauge_tgt_battery_status();

    /* Initialize the ADC readings to something usable */
    fgs.last_adc_battery_status = FUELGAUGE_BATTERY_UNKNOWN;
    fgs.v_batt_last = percent_to_volt_discharge[0][10];
    fgs.i_batt_last = 0;
    fgs.p_batt_last = 0;
    fgs.temp_last = fuelgauge_tgt_read_temperature();
    fgs.temp_sum = fgs.temp_last * FUELGAUGE_ADC_SAMPLES;
    fuelgauge_step_adc();

    /* Load fuelgauge data */
    fuelgauge_reset();
    fuelgauge_tgt_load_data(&fgd);

    /* Initialize system state */
    fgs.charge = 0;
    if(fgs.cur_battery_status == FUELGAUGE_BATTERY_FULL)
        fgs.state = FUELGAUGE_STATE_FULL;
    else if(fgs.v_batt < battery_level_dangerous[0] + 50)
        fgs.state = FUELGAUGE_STATE_EMPTY;
    else
        fgs.state = FUELGAUGE_STATE_NORMAL;
}

void charging_algorithm_step(void)
{
    fuelgauge_tgt_step();

    fgs.cur_battery_status = fuelgauge_tgt_battery_status();
    fuelgauge_step_adc();

    /* Read charge and update charge limits */
    bool need_clear_charge = false;
    fgs.charge = fuelgauge_tgt_read_charge();

    if(fgs.charge > fgd.charge_max) {
        fgd.charge_max = fgs.charge;
        fgs.dirty |= FUELGAUGE_DATA_CHARGE_LIMITS;
    }

    if(fgs.charge < fgd.charge_min) {
        fgd.charge_min = fgs.charge;
        fgs.dirty |= FUELGAUGE_DATA_CHARGE_LIMITS;
    }

    /* Handle state transition actions */
    switch(fgs.state) {
    case FUELGAUGE_STATE_NORMAL: {
        if(fgs.cur_battery_status == FUELGAUGE_BATTERY_FULL) {
            logf("fuelgauge: NORMAL -> FULL state");
            fgd.charge_max = 0;
            fgd.charge_min -= fgs.charge;
            fgs.dirty |= FUELGAUGE_DATA_CHARGE_LIMITS;
            need_clear_charge = true;
            fgs.state = FUELGAUGE_STATE_FULL;
        } else if(fgs.v_batt > 0 && fgs.v_batt < battery_level_dangerous[0] &&
                  fgs.cur_battery_status == FUELGAUGE_BATTERY_DISCHARGING) {
            logf("fuelgauge: NORMAL -> EMPTY state");
            fgd.charge_min = 0;
            fgd.charge_max -= fgs.charge;
            fgs.dirty |= FUELGAUGE_DATA_CHARGE_LIMITS;
            need_clear_charge = true;
            fgs.state = FUELGAUGE_STATE_FULL;
        }
    } break;

    case FUELGAUGE_STATE_FULL: {
        if(fgs.cur_battery_status != FUELGAUGE_BATTERY_FULL) {
            logf("fuelgauge: FULL -> NORMAL state");
            fgs.state = FUELGAUGE_STATE_NORMAL;
        }
    } break;

    case FUELGAUGE_STATE_EMPTY: {
        if(fgs.v_batt > 0 && fgs.v_batt > battery_level_dangerous[0] + 50 &&
           fgs.cur_battery_status == FUELGAUGE_BATTERY_CHARGING) {
            logf("fuelgauge: EMPTY -> NORMAL state");
            fgs.state = FUELGAUGE_STATE_NORMAL;
        }

        /* Don't save data at this time to avoid corrupting
         * it in the event of a power failure. */
        fgs.dirty = 0;
    } break;

    default:
        panicf("fuelgauge: unhandled state (%d)", fgs.state);
        break;
    }

    if(fgs.dirty) {
        fuelgauge_tgt_save_data(fgs.dirty, &fgd);
        fgs.dirty = 0;
    }

    if(need_clear_charge) {
        fuelgauge_tgt_clear_charge();
        fgs.charge = 0;
    }
}

void charging_algorithm_close(void)
{
    /* XXX: Do close-related tasks */

    fuelgauge_tgt_shutdown();
}

void fuelgauge_reset(void)
{
    fgd.charge_min = 0;
    fgd.charge_max = 0;
    fgs.dirty = FUELGAUGE_DATA_ALL;
}

int fuelgauge_state(void)
{
    return fgs.state;
}

static long fuelgauge_ratio_soc(long now, long full)
{
    if(full == 0)
        return -1;

    long pct = now * 100 / full;
    pct = MAX(1, pct);
    pct = MIN(pct, 99);

    switch(fgs.state) {
    case FUELGAUGE_STATE_EMPTY:
        return -1;
    case FUELGAUGE_STATE_FULL:
        return 100;
    case FUELGAUGE_STATE_NORMAL:
    default:
        return pct;
    }
}

long fuelgauge_vsoc(void)
{
    /* TODO: this should be identical to the calculation in powermgmt.c */
    return 50;
}

long fuelgauge_isoc(void)
{
    long now = fuelgauge_charge_now();
    long full = fuelgauge_charge_full();
    return fuelgauge_ratio_soc(now, full);
}

long fuelgauge_soe(void)
{
    long now = fuelgauge_energy_now();
    long full = fuelgauge_energy_full();
    return fuelgauge_ratio_soc(now, full);
}

long fuelgauge_soh(void)
{
    /* TODO: implement SoH. It's SoH = capacity divided by design capacity */
    return 100;
}

long fuelgauge_charge_now(void)
{
    return fgs.charge - fgd.charge_min;
}

long fuelgauge_charge_full(void)
{
    return fgd.charge_max - fgd.charge_min;
}

long fuelgauge_current_now(void)
{
    return fgs.i_batt;
}

/* TODO: perform SoE calculations
 *
 * It's just a piecewise linear integration of charge * voltage, so it's
 * not too complicated.
 *
 * Basically for each increment of charge, starting at 0, we look up the
 * voltage corresponding to this SoC (by interpolating) and add it up.
 * At the end we divide the whole thing by charge_now or charge_full,
 * depending on which one we're integrating over.
 */

long fuelgauge_energy_now(void)
{
    return fuelgauge_charge_now();
}

long fuelgauge_energy_full(void)
{
    return fuelgauge_charge_full();
}

long fuelgauge_power_now(void)
{
    return fgs.p_batt;
}

#if (FUELGAUGE_CAPS & FUELGAUGE_CAP_POWER) == 0
# error "TODO"
#endif

#if (FUELGAUGE_CAPS & FUELGAUGE_CAP_TEMPERATURE) == 0
# error "TODO"
#endif

bool charging_state(void)
{
    return fgs.cur_battery_status == FUELGAUGE_BATTERY_CHARGING;
}

#if (CONFIG_BATTERY_MEASURE & VOLTAGE_MEASURE)
int _battery_voltage(void)
{
    return fgs.v_batt;
}
#endif

#if (CONFIG_BATTERY_MEASURE & PERCENTAGE_MEASURE)
int _battery_level(void)
{
    return 50;
}
#endif

#if (CONFIG_BATTERY_MEASURE & TIME_MEASURE)
int _battery_time(void)
{
    return 60;
}
#endif

#ifndef BOOTLOADER

static const char* fuelgauge_dbg_batt_status_to_str(int status)
{
    switch(status) {
    case FUELGAUGE_BATTERY_DISCHARGING:
    default:
        return "Discharging";
    case FUELGAUGE_BATTERY_CHARGING:
        return "Charging";
    case FUELGAUGE_BATTERY_FULL:
        return "Full";
    }
}

static const char* fuelgauge_dbg_batt_status(void)
{
    return fuelgauge_dbg_batt_status_to_str(fgs.cur_battery_status);
}

static const char* fuelgauge_dbg_adc_batt_status(void)
{
    return fuelgauge_dbg_batt_status_to_str(fgs.last_adc_battery_status);
}

static const char* fuelgauge_dbg_state(void)
{
    switch(fgs.state) {
    default:
    case FUELGAUGE_STATE_NORMAL:
        return "Normal";
    case FUELGAUGE_STATE_FULL:
        return "Full";
    case FUELGAUGE_STATE_EMPTY:
        return "Empty";
    }
}

static long fuelgauge_dbg_batt_voltage(void)
{
    return fgs.v_batt;
}

static long fuelgauge_dbg_batt_current(void)
{
    return fgs.i_batt;
}

static long fuelgauge_dbg_batt_power(void)
{
    return fgs.p_batt;
}

static long fuelgauge_dbg_batt_temp(void)
{
    return fgs.temp;
}

static long fuelgauge_dbg_min_charge(void)
{
    return fgd.charge_min;
}

static long fuelgauge_dbg_max_charge(void)
{
    return fgd.charge_max;
}

static long fuelgauge_dbg_cur_charge(void)
{
    return fgs.charge;
}

static void fuelgauge_dbg_reset_state(void)
{
    fuelgauge_reset();
}

static const struct fg_debug_menu_entry {
    const char* name;
    long(*get_value)(void);
    const char*(*get_enum)(void);
    void(*action)(void);
} fg_debug_menuentries[] = {
    {"vSOC",            fuelgauge_vsoc, NULL, NULL},
    {"iSOC",            fuelgauge_isoc, NULL, NULL},
    {"SOE",             fuelgauge_soe, NULL, NULL},
    {"SOH",             fuelgauge_soh, NULL, NULL},
    {"Charge now",      fuelgauge_charge_now, NULL, NULL},
    {"Charge full",     fuelgauge_charge_full, NULL, NULL},
    {"Energy now",      fuelgauge_energy_now, NULL, NULL},
    {"Energy full",     fuelgauge_energy_full, NULL, NULL},
    {"Battery voltage", fuelgauge_dbg_batt_voltage, NULL, NULL},
    {"Battery current", fuelgauge_dbg_batt_current, NULL, NULL},
    {"Battery power",   fuelgauge_dbg_batt_power, NULL, NULL},
    {"Battery temperature", fuelgauge_dbg_batt_temp, NULL, NULL},
    {"Battery status",  NULL, fuelgauge_dbg_batt_status, NULL},
    {"ADC batt status", NULL, fuelgauge_dbg_adc_batt_status, NULL},
    {"FSM state",       NULL, fuelgauge_dbg_state, NULL},
    {"Min charge",      fuelgauge_dbg_min_charge, NULL, NULL},
    {"Max charge",      fuelgauge_dbg_max_charge, NULL, NULL},
    {"Cur charge",      fuelgauge_dbg_cur_charge, NULL, NULL},
    {"Reset state",     NULL, NULL, fuelgauge_dbg_reset_state},
};

static int fuelgauge_debug_menu_cb(int action, struct gui_synclist* lists)
{
    if(action == ACTION_STD_OK) {
        int sel = gui_synclist_get_sel_pos(lists);
        const struct fg_debug_menu_entry* entry = &fg_debug_menuentries[sel];
        if(entry->action)
            entry->action();
        action = ACTION_REDRAW;
    } else if(action == ACTION_NONE)
        action = ACTION_REDRAW;

    return action;
}

static const char* fuelgauge_debug_menu_get_name(int item, void* data,
                                                 char* buf, size_t buflen)
{
    (void)data;

    const struct fg_debug_menu_entry* entry = &fg_debug_menuentries[item];
    if(entry->get_value)
        snprintf(buf, buflen, "%s: %ld", entry->name, entry->get_value());
    else if(entry->get_enum)
        snprintf(buf, buflen, "%s: %s", entry->name, entry->get_enum());
    else
        snprintf(buf, buflen, "%s", entry->name);

    return buf;
}

bool fuelgauge_debug_menu(void)
{
    struct simplelist_info info;
    simplelist_info_init(&info, "Fuelgauge debug",
                         ARRAYLEN(fg_debug_menuentries), NULL);
    info.action_callback = fuelgauge_debug_menu_cb;
    info.get_name = fuelgauge_debug_menu_get_name;
    return simplelist_show_list(&info);
}
#endif

/* OVERVIEW and TODO
 * -----------------
 *
 * State-of-charge (SoC) tracking is implemented by tracking the net charge
 * flow in/out of the battery since the last reset of the coulomb counters.
 * Net charge is defined as 'net = charge - discharge', and can be positive
 * or negative.
 *
 * To infer the capacity we keep values charge_min / charge_max which are
 * bounds on the min/max value of net charge we've seen. The capacity is
 * given by 'charge_max - charge_min', and the current level of charge is
 * given by 'net - charge_min'.
 *
 * When the battery is full or empty, we recalibrate charge_min / charge_max.
 * If our estimate of the capacity is too high, then the charge level will
 * not be exactly 0% or 100%, so we subtract the discrepancy from our capacity.
 * This is the only part of the algorithm which can reduce estimated capacity.
 * Coulomb counters also get reset to 0 at this time.
 *
 * This simplistic algorithm has some major drawbacks:
 *
 * - Coulomb counting is inherently inaccurate due to measurement errors.
 *   The relative measurement error is greater at low currents, and low
 *   ADC sampling rates will miss transient variations in the current.
 *
 * - Capacity loss isn't taken into account except when the battery is full
 *   or empty. If neither happens for a long time, the capacity loss may be
 *   significant enough that when a full/empty event finally occurs, a large
 *   adjustment is made at once which is disconcerting to the user.
 *
 * - Battery capacity is dependent on the rate of discharge: this is known
 *   as the Peukert effect. Temperature and other environmental factors will
 *   also affect the capacity.
 *
 * - State of energy != state of charge. The amount of energy available to
 *   power the device is dependent on terminal voltage as well as current.
 *   This is only a reporting problem: we can calculate SoE from SoC if we
 *   have a voltage - SoC curve to integrate over.
 *
 * To improve things we need to combine the coulomb counting SoC estimate
 * with a voltage-based SoC estimate.
 *
 * Terminal voltage can estimate the SoC since terminal voltage approximates
 * the open-circuit voltage, and the OCV has a 1-to-1 relationship with SoC.
 * Unfortunately small errors in the OCV magnify to large SoC errors because
 * lithium batteries have very flat OCV - SoC curves for 80-90% of the usable
 * voltage range. Errors are further magnified because we use the terminal
 * voltage instead of OCV; the terminal voltage varies depending on discharge
 * current, temperature, and other factors.
 *
 * My initial idea is to compare the voltage SoC (V-SoC) estimate to the
 * charge SoC (C-SoC) estimate. If they differ widely, then C-SoC must be
 * wrong and min/max charge limits should be shifted so the capacity moves
 * closer to what V-SoC suggests. We also have to ignore V-SoC during the
 * constant voltage phase of charging, since the V-SoC doesn't carry any
 * useful information at this time.
 *
 * Using PID control to correct the error is the simplest and should be
 * the most effective. The right parameters could be found through testing.
 * This isn't perfect, but will hopefully suffce for Rockbox's needs.
 */
