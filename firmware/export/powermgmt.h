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
#define BATTERY_CAPACITY_MIN 2200
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable in settings */
#define BATTERY_TYPES_COUNT      1
#elif CONFIG_BATTERY == BATT_3AAA /* Ondio */
#define BATTERY_CAPACITY_MIN 500
#define BATTERY_CAPACITY_MAX 1500 /* max. capacity selectable in settings */
#define BATTERY_TYPES_COUNT      2   /* Alkalines or NiMH */
#elif CONFIG_BATTERY == BATT_LIPOL1300 /* iRiver H1x0 */
#define BATTERY_CAPACITY_MIN 1300
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable in settings */
#define BATTERY_TYPES_COUNT      1
#else /* Recorder, NiMH */
#define BATTERY_CAPACITY_MIN 1500
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable in settings */
#define BATTERY_TYPES_COUNT      1
#endif

#define POWER_HISTORY_LEN 2*60   /* 2 hours of samples, one per minute */

#define CHARGE_END_NEGD   6      /* stop when N minutes have passed with
                                  * avg delta being < -0.05 V */
#define CHARGE_END_ZEROD  50     /* stop when N minutes have passed with
                                  * avg delta being < 0.005 V */

#ifndef SIMULATOR

#ifdef HAVE_CHARGE_CTRL
#define START_TOPOFF_CHG    85  /* Battery % to start at top-off */
#define START_TRICKLE_CHG   95  /* Battery % to start at trickle */

#define POWER_MESSAGE_LEN 32     /* power thread status message */
#define CHARGE_MAX_TIME_1500 450 /* minutes: maximum charging time for 1500 mAh batteries */
                                 /* actual max time depends also on BATTERY_CAPACITY! */
#define CHARGE_MIN_TIME   10     /* minutes: minimum charging time */
#define TOPOFF_MAX_TIME   90     /* After charging, go to top off charge. How long should top off charge be? */
#define TOPOFF_VOLTAGE    565    /* which voltage is best? (centivolts) */
#define TRICKLE_MAX_TIME  12*60  /* After top off charge, go to trickle charge. How long should trickle charge be? */
#define TRICKLE_VOLTAGE   545    /* which voltage is best? (centivolts) */

#define START_TOPOFF_SEC    25   /* initial trickle_sec for topoff */
#define START_TRICKLE_SEC   15   /* initial trickle_sec for trickle */

#define PID_PCONST          2   /* PID proportional constant */
#define PID_DEADZONE        2   /* PID proportional deadzone */

extern char power_message[POWER_MESSAGE_LEN];

extern int long_delta;          /* long term delta battery voltage */
extern int short_delta;         /* short term delta battery voltage */

extern int powermgmt_last_cycle_startstop_min; /* how many minutes ago was the charging started or stopped? */
extern int powermgmt_last_cycle_level;         /* which level had the batteries at this time? */

extern int pid_p;                /* PID proportional term */
extern int pid_i;                /* PID integral term */
extern int trickle_sec;          /* trickle charge: How many seconds per minute are we charging actually? */

#endif /* HAVE_CHARGE_CTRL */

#if defined(HAVE_CHARGE_CTRL) || (CONFIG_BATTERY == BATT_LIION2200)
typedef enum {
    DISCHARGING,
    CHARGING,
    TOPOFF,
    TRICKLE
} charge_state_type;

/* tells what the charger is doing */
extern charge_state_type charge_state;
#endif /* defined(HAVE_CHARGE_CTRL) || (CONFIG_BATTERY == BATT_LIION2200) */

#ifdef HAVE_MMC  /* Values for Ondio */
#define CURRENT_NORMAL     95  /* average, nearly proportional to 1/U */
#define CURRENT_USB         1  /* host powered in USB mode; avoid zero-div */
#define CURRENT_BACKLIGHT   0  /* no backlight */
#else            /* Values for HD based jukeboxes */
#define CURRENT_NORMAL    145  /* usual current in mA when using the AJB including some disk/backlight/... activity */
#define CURRENT_USB       500  /* usual current in mA in USB mode */
#define CURRENT_BACKLIGHT  30  /* additional current when backlight is always on */

#define CURRENT_MIN_CHG    70  /* minimum charge current */
#define MIN_CHG_V        8500  /* at 8.5v charger voltage get CURRENT_MIN_CHG */
#define CURRENT_MAX_CHG   350  /* maximum charging current */
#define MAX_CHG_V       10250  /* anything over 10.25v gives CURRENT_MAX_CHG */
#endif /* HAVE_MMC */

extern unsigned int bat;       /* filtered battery voltage, centivolts */
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
void set_battery_type(int type);         /* set local battery type */

void set_sleep_timer(int seconds);
int get_sleep_timer(void);
void set_car_adapter_mode(bool setting);
void reset_poweroff_timer(void);
void shutdown_hw(void);

#endif
