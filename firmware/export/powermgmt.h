/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Heikki Hannikainen, Uwe Freese
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
#ifndef _POWERMGMT_H_
#define _POWERMGMT_H_

#include <stdbool.h>
#include "config.h"

#define POWER_HISTORY_LEN 2*60   /* 2 hours of samples, one per minute */

enum charge_state_type
{
    /* sorted by increasing charging current (do not change!) */
#if CONFIG_CHARGING >= CHARGING_MONITOR
    CHARGE_STATE_DISABLED = -2, /* Disable charger use (safety measure) */
    CHARGE_STATE_ERROR = -1,    /* Some error occurred that should not allow
                                   turning on the charger again by software
                                   without user intervention (ie. replug) */
#endif
    DISCHARGING = 0,
#if CONFIG_CHARGING >= CHARGING_MONITOR
    TRICKLE,         /* For NiCd, battery maintenence phase */
                     /* For LiIon, low-current precharge phase */
    TOPOFF,          /* For NiCd, waiting for dead zone */
                     /* For LiIon, constant voltage phase */
    CHARGING,        /* For NiCd, main charge phase */
                     /* For LiIon, constant current phase */
#endif
};

/* tells what the charger is doing */
extern enum charge_state_type charge_state;

#if CONFIG_CHARGING
/*
 * Flag that the charger has been plugged in/removed: this is set for exactly
 * one time through the power loop when the charger has been plugged in.
 */
enum charger_input_state_type
{
    NO_CHARGER = 0,     /* No charger is present */
    CHARGER_UNPLUGGED,  /* Transitional state during CHARGER=>NO_CHARGER */
    CHARGER_PLUGGED,    /* Transitional state during NO_CHARGER=>CHARGER */
    CHARGER             /* Charger is present */
};

/* tells the state of the charge input */
extern enum charger_input_state_type charger_input_state;

/* Power input status saved on the power thread each loop */
extern unsigned int power_thread_inputs;

#endif /* CONFIG_CHARGING */

enum shutdown_type
{
    SHUTDOWN_POWER_OFF,
    SHUTDOWN_REBOOT,
};

#if CONFIG_CHARGING == CHARGING_TARGET
/* Include target-specific definitions */
#include "powermgmt-target.h"
#endif

/* Start up power management thread */
void powermgmt_init(void) INIT_ATTR;

/* Generic current values that are intentionally meaningless - config header
 * should define proper numbers.*/

#ifndef CURRENT_USB
#define CURRENT_USB       2 /* usual current in mA in USB mode */
#endif

#if CONFIG_CHARGING && !defined(CURRENT_MAX_CHG)
#define CURRENT_MAX_CHG   350  /* maximum charging current */
#endif

#ifndef BATT_AVE_SAMPLES
/* slw filter constant unless otherwise specified */
#define BATT_AVE_SAMPLES 128
#endif

#ifndef BATT_CURRENT_AVE_SAMPLES
/* semi arbitrary but needs to be 'large' for the time estimation algorithm */
#define BATT_CURRENT_AVE_SAMPLES 128
#endif

#ifndef POWER_THREAD_STEP_TICKS
/* 2HZ sample rate unless otherwise specified */
#define POWER_THREAD_STEP_TICKS (HZ/2)
#endif

extern unsigned short power_history[POWER_HISTORY_LEN];
extern const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT];
extern const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT];
extern const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11];
#if CONFIG_CHARGING
extern const unsigned short percent_to_volt_charge[11];
#endif

/* Returns battery status, filtered for runtime estimation */
int battery_level(void); /* percent */
int battery_time(void); /* minutes */
int battery_voltage(void); /* filtered batt. voltage in millivolts */
int battery_current(void); /* battery current in milliamps
                            * (may just be a rough estimate) */

/* Implemented by the target, unfiltered */
int _battery_level(void); /* percent */
int _battery_time(void); /* minutes */
int _battery_voltage(void); /* voltage in millivolts */
int _battery_current(void); /* (dis)charge current in milliamps */

#if CONFIG_CHARGING >= CHARGING_TARGET
void powermgmt_init_target(void);
void charging_algorithm_close(void);
void charging_algorithm_step(void);
#endif

#ifdef HAVE_BATTERY_SWITCH
unsigned int input_millivolts(void); /* voltage that device is running from */
#endif /* HAVE_BATTERY_SWITCH */

#if defined(HAVE_BATTERY_SWITCH) || defined(HAVE_RESET_BATTERY_FILTER)
/* Set the filtered battery voltage (to adjust it before beginning a charge
 * cycle for instance where old, loaded readings will likely be invalid).
 * Also readjust when battery switch is opened or closed.
 */
void reset_battery_filter(int millivolts);
#endif /* HAVE_BATTERY_SWITCH || HAVE_RESET_BATTERY_FILTER */


/* read unfiltered battery info */
void battery_read_info(int *voltage, int *level);

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void);

void set_poweroff_timeout(int timeout);
#if BATTERY_CAPACITY_INC > 0
void set_battery_capacity(int capacity); /* set local battery capacity value */
#endif
int get_battery_capacity(void); /* get local battery capacity value */
void set_battery_type(int type); /* set local battery type */

void set_sleeptimer_duration(int minutes);
int get_sleep_timer(void);
void set_keypress_restarts_sleep_timer(bool enable);
void handle_auto_poweroff(void);
void set_car_adapter_mode(bool setting);
void reset_poweroff_timer(void);
void cancel_shutdown(void);
void shutdown_hw(enum shutdown_type sd_type);
void sys_poweroff(void);
void sys_reboot(void);
/* Returns true if the system should force shutdown for some reason -
 * eg. low battery */
bool query_force_shutdown(void);
#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool);
#endif
#ifdef HAVE_LINEOUT_POWEROFF
void lineout_set(bool);
#endif

#endif /* _POWERMGMT_H_ */
