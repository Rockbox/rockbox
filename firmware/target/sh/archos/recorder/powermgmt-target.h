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
 * Revisions copyright (C) 2005 by Gerald Van Baren
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
#ifndef POWERMGMT_TARGET_H
#define POWERMGMT_TARGET_H

/*
 * Define CHARGING_DEBUG_FILE to create a csv (spreadsheet) with battery
 * information in it (one sample per minute/connect/disconnect).
 *
 * This is only for very low level debug.
 */
#undef CHARGING_DEBUG_FILE


/* stop when N minutes have passed with avg delta being < -0.05 V */
#define CHARGE_END_SHORTD       6
/* stop when N minutes have passed with avg delta being < -0.02 V */
#define CHARGE_END_LONGD       50

/* Battery % to start at top-off */
#define START_TOPOFF_CHG       85
/* Battery % to start at trickle */
#define START_TRICKLE_CHG      95
/* power thread status message */
#define POWER_MESSAGE_LEN      32
/* minutes: maximum charging time for 1500 mAh batteries
 * actual max time depends also on BATTERY_CAPACITY! */
#define CHARGE_MAX_MIN_1500   450
/* minutes: minimum charging time */     
#define CHARGE_MIN_MIN         10
/* After charging, go to top off charge. How long should top off charge be? */
#define TOPOFF_MAX_MIN         90
/* which voltage is best? (millivolts) */
#define TOPOFF_VOLTAGE       5650
/* After top off charge, go to trickle harge. How long should trickle
 * charge be? */
#define TRICKLE_MAX_MIN       720 /* 12 hrs */
/* which voltage is best? (millivolts) */
#define TRICKLE_VOLTAGE      5450
/* initial trickle_sec for topoff */
#define START_TOPOFF_SEC       25
/* initial trickle_sec for trickle */
#define START_TRICKLE_SEC      15

#define PID_DEADZONE            4    /* PID proportional deadzone */

extern char power_message[POWER_MESSAGE_LEN];

extern int long_delta;          /* long term delta battery voltage */
extern int short_delta;         /* short term delta battery voltage */

extern int powermgmt_last_cycle_startstop_min; /* how many minutes ago was
                                                  the charging started or
                                                  stopped? */
extern int powermgmt_last_cycle_level;         /* which level had the batteries
                                                  at this time? */

extern int pid_p;                /* PID proportional term */
extern int pid_i;                /* PID integral term */
extern int trickle_sec;          /* how many seconds should the
                                    charger be enabled per
                                    minute for trickle
                                    charging? */
void charger_enable(bool on);
bool charger_enabled(void);

/* Battery filter lengths in samples */
#define BATT_AVE_SAMPLES 32

/* No init to do */
static inline void powermgmt_init_target(void) {}
void charging_algorithm_step(void);

#ifdef CHARGING_DEBUG_FILE
/* Need to flush and close debug file */
void charging_algorithm_close(void);
#else
/* No poweroff operation to do */
static inline void charging_algorithm_close(void) {}
#endif

#endif /* POWERMGMT_TARGET_H */
