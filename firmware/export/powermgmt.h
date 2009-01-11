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

#ifdef CONFIG_CHARGING
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

#if CONFIG_CHARGING == CHARGING_TARGET
/* Include target-specific definitions */
#include "powermgmt-target.h"
#endif

#ifndef SIMULATOR

/* Generic current values that are really rather meaningless - config header
 * should define proper numbers. */
#ifndef CURRENT_NORMAL
#define CURRENT_NORMAL    145  /* usual current in mA */
#endif

#ifndef CURRENT_BACKLIGHT
#define CURRENT_BACKLIGHT  30  /* additional current when backlight always on */
#endif

#ifdef HAVE_RECORDING
#ifndef CURRENT_RECORD
#define CURRENT_RECORD     35  /* additional recording current */
#endif
#endif /* HAVE_RECORDING */

#ifndef CURRENT_USB
#define CURRENT_USB       500 /* usual current in mA in USB mode */
#endif

#ifdef HAVE_REMOTE_LCD
#define CURRENT_REMOTE      8  /* additional current when remote connected */
#endif /* HAVE_REMOTE_LCD */

#if CONFIG_CHARGING
#ifndef CURRENT_MAX_CHG
#define CURRENT_MAX_CHG   350  /* maximum charging current */
#endif
#endif /* CONFIG_CHARGING */

#ifdef CHARGING_DEBUG_FILE
#define POWERMGMT_DEBUG_STACK ((0x1000)/sizeof(long))
#else
#define POWERMGMT_DEBUG_STACK 0
#endif

#ifndef BATT_AVE_SAMPLES
/* slw filter constant unless otherwise specified */
#define BATT_AVE_SAMPLES 128
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

/* Start up power management thread */
void powermgmt_init(void);

#endif /* SIMULATOR */

/* Returns battery statust */
int battery_level(void); /* percent */
int battery_time(void); /* minutes */
unsigned int battery_adc_voltage(void); /* voltage from ADC in millivolts */
unsigned int battery_voltage(void); /* filtered batt. voltage in millivolts */

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
void set_battery_capacity(int capacity); /* set local battery capacity value */
int  get_battery_capacity(void); /* get local battery capacity value */
void set_battery_type(int type); /* set local battery type */

void set_sleep_timer(int seconds);
int get_sleep_timer(void);
void set_car_adapter_mode(bool setting);
void reset_poweroff_timer(void);
void cancel_shutdown(void);
void shutdown_hw(void);
void sys_poweroff(void);
/* Returns true if the system should force shutdown for some reason -
 * eg. low battery */
bool query_force_shutdown(void);
#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool);
#endif

#endif /* _POWERMGMT_H_ */
