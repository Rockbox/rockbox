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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _POWERMGMT_H_
#define _POWERMGMT_H_

#if CONFIG_BATTERY == BATT_LIION2200 /* FM Recorder, LiIon */
#define BATTERY_LEVEL_SHUTDOWN   260 /* 2.60V */
#define BATTERY_LEVEL_EMPTY      265 /* 2.65V */
#define BATTERY_LEVEL_DANGEROUS  280 /* 2.80V */
#define BATTERY_LEVEL_FULL       400 /* 4.00V */
#elif CONFIG_BATTERY == BATT_3AAA_ALKALINE /* Ondio, Alkalines */
#define BATTERY_LEVEL_SHUTDOWN   250 /* 2.50V */
#define BATTERY_LEVEL_EMPTY      260 /* 2.60V */
#define BATTERY_LEVEL_DANGEROUS  270 /* 2.80V */
#define BATTERY_LEVEL_FULL       450 /* 4.50V */
#else /* Recorder, NiMH */
#define BATTERY_LEVEL_SHUTDOWN   450 /* 4.50V */
#define BATTERY_LEVEL_EMPTY      465 /* 4.65V */
#define BATTERY_LEVEL_DANGEROUS  475 /* 4.75V */
#define BATTERY_LEVEL_FULL       585 /* 5.85V */
#endif

#define BATTERY_RANGE (BATTERY_LEVEL_FULL - BATTERY_LEVEL_EMPTY)
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity that can be selected in settings menu, min. is always 1500 */

#define POWER_HISTORY_LEN 2*60   /* 2 hours of samples, one per minute */
#define POWER_AVG_N       4      /* how many samples to take for each measurement */
#define POWER_AVG_SLEEP   9      /* how long do we sleep between each measurement */

#define CHARGE_END_NEGD   6      /* stop when N minutes have passed with
                                  * avg delta being < -0.05 V */
#define CHARGE_END_ZEROD  50     /* stop when N minutes have passed with
                                  * avg delta being < 0.005 V */
#ifndef SIMULATOR

#ifdef HAVE_CHARGE_CTRL
#define POWER_MESSAGE_LEN 32     /* power thread status message */
#define CHARGE_MAX_TIME_1500 450 /* minutes: maximum charging time for 1500 mAh batteries */
                                 /* actual max time depends also on BATTERY_CAPACITY! */
#define CHARGE_MIN_TIME   10     /* minutes: minimum charging time */
#define CHARGE_RESTART_HI 85     /* %: when to restart charging in 'charge' mode */
                                 /* attention: if set too high, normal charging is started in trickle mode */
#define CHARGE_RESTART_LO 10     /* %: when to restart charging in 'discharge' mode */
#define CHARGE_PAUSE_LEN  60     /* how many minutes to pause between charging cycles */
#define TOPOFF_MAX_TIME   90     /* After charging, go to top off charge. How long should top off charge be? */
#define TOPOFF_VOLTAGE    565    /* which voltage is best? (centivolts) */
#define TRICKLE_MAX_TIME  12*60  /* After top off charge, go to trickle charge. How long should trickle charge be? */
#define TRICKLE_VOLTAGE   545    /* which voltage is best? (centivolts) */

extern char power_message[POWER_MESSAGE_LEN];
extern char charge_restart_level;

extern int powermgmt_last_cycle_startstop_min; /* how many minutes ago was the charging started or stopped? */
extern int powermgmt_last_cycle_level;         /* which level had the batteries at this time? */

extern int battery_lazyness[20]; /* how does the battery react when plugging in/out the charger */
void enable_trickle_charge(bool on);
extern int trickle_sec;          /* trickle charge: How many seconds per minute are we charging actually? */

#endif /* HAVE_CHARGE_CTRL */

#if defined(HAVE_CHARGE_CTRL) || CONFIG_BATTERY == BATT_LIION2200
extern int charge_state;         /* tells what the charger is doing (for info display): 0: decharging/charger off, 1: charge, 2: top-off, 3: trickle */
#endif

#define CURRENT_NORMAL    145    /* usual current in mA when using the AJB including some disk/backlight/... activity */
#define CURRENT_USB       500    /* usual current in mA in USB mode */
#define CURRENT_BACKLIGHT  30    /* additional current when backlight is always on */
#define CURRENT_CHARGING  300    /* charging current */

extern unsigned short power_history[POWER_HISTORY_LEN];

/* Start up power management thread */
void powermgmt_init(void);

#endif /* SIMULATOR */

/* Returns battery level in percent */
int battery_level(void);
int battery_time(void); /* minutes */

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void);

void set_poweroff_timeout(int timeout);
void set_battery_capacity(int capacity); /* set local battery capacity value */

void set_sleep_timer(int seconds);
int get_sleep_timer(void);
void set_car_adapter_mode(bool setting);
void reset_poweroff_timer(void);

#endif
