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

#include <stdbool.h>

#define POWER_HISTORY_LEN 2*60   /* 2 hours of samples, one per minute */

#define CHARGE_END_SHORTD  6     /* stop when N minutes have passed with
                                  * avg delta being < -0.05 V */
#define CHARGE_END_LONGD  50     /* stop when N minutes have passed with
                                  * avg delta being < -0.02 V */

#if CONFIG_CHARGING >= CHARGING_MONITOR
typedef enum {       /* sorted by increasing charging current */
    DISCHARGING = 0,
    TRICKLE,         /* Can occur for CONFIG_CHARGING >= CHARGING_MONITOR */
    TOPOFF,          /* Can occur for CONFIG_CHARGING == CHARGING_CONTROL */
    CHARGING         /* Can occur for all CONFIG_CHARGING options */
} charge_state_type;

/* tells what the charger is doing */
extern charge_state_type charge_state;
#endif /* CONFIG_CHARGING >= CHARGING_MONITOR */

#ifdef CONFIG_CHARGING
/*
 * Flag that the charger has been plugged in/removed: this is set for exactly
 * one time through the power loop when the charger has been plugged in.
 */
typedef enum {
    NO_CHARGER,
    CHARGER_UNPLUGGED,              /* transient state */
    CHARGER_PLUGGED,                /* transient state */
    CHARGER
} charger_input_state_type;

/* tells the state of the charge input */
extern charger_input_state_type charger_input_state;
#endif

#ifndef SIMULATOR

#if CONFIG_CHARGING == CHARGING_CONTROL
#define START_TOPOFF_CHG    85  /* Battery % to start at top-off */
#define START_TRICKLE_CHG   95  /* Battery % to start at trickle */

#define POWER_MESSAGE_LEN 32     /* power thread status message */
#define CHARGE_MAX_TIME_1500 450 /* minutes: maximum charging time for 1500 mAh batteries */
                                 /* actual max time depends also on BATTERY_CAPACITY! */
#define CHARGE_MIN_TIME   10     /* minutes: minimum charging time */
#define TOPOFF_MAX_TIME   90     /* After charging, go to top off charge. How long should top off charge be? */
#define TOPOFF_VOLTAGE    5650   /* which voltage is best? (millivolts) */
#define TRICKLE_MAX_TIME  12*60  /* After top off charge, go to trickle charge. How long should trickle charge be? */
#define TRICKLE_VOLTAGE   5450   /* which voltage is best? (millivolts) */

#define START_TOPOFF_SEC    25   /* initial trickle_sec for topoff */
#define START_TRICKLE_SEC   15   /* initial trickle_sec for trickle */

#define PID_DEADZONE        4    /* PID proportional deadzone */

extern char power_message[POWER_MESSAGE_LEN];

extern int long_delta;          /* long term delta battery voltage */
extern int short_delta;         /* short term delta battery voltage */

extern int powermgmt_last_cycle_startstop_min; /* how many minutes ago was the charging started or stopped? */
extern int powermgmt_last_cycle_level;         /* which level had the batteries at this time? */

extern int pid_p;                /* PID proportional term */
extern int pid_i;                /* PID integral term */
extern int trickle_sec;          /* trickle charge: How many seconds per minute are we charging actually? */

#endif /* CONFIG_CHARGING == CHARGING_CONTROL */

#ifdef HAVE_MMC  /* Values for Ondio */
# define CURRENT_NORMAL     95  /* average, nearly proportional to 1/U */
# define CURRENT_USB         1  /* host powered in USB mode; avoid zero-div */
# define CURRENT_BACKLIGHT   0  /* no backlight */
#else            /* Values for HD based jukeboxes */
#ifdef IRIVER_H100_SERIES
# define CURRENT_NORMAL     80  /* 16h playback on 1300mAh battery */
# define CURRENT_BACKLIGHT  23  /* from IriverBattery twiki page */
# define CURRENT_SPDIF_OUT  10  /* optical SPDIF output on */
# define CURRENT_RECORD    105  /* additional current while recording */
#elif defined(IRIVER_H300_SERIES)
# define CURRENT_NORMAL     80  /* 16h playback on 1300mAh battery from IriverRuntime wiki page */
# define CURRENT_BACKLIGHT  23  /* FIXME: This needs to be measured, copied from H100 */
# define CURRENT_RECORD    110  /* additional current while recording */
#elif defined(IPOD_ARCH) && (MODEL_NUMBER==4)   /* iPOD Nano */
# define CURRENT_NORMAL     35  /* 8.5-9.0h playback out of 300mAh battery from IpodRuntime */
# define CURRENT_BACKLIGHT  20  /* FIXME: estimation took over from iPOD Video */
#if defined(HAVE_RECORDING)
# define CURRENT_RECORD     35  /* FIXME: this needs adjusting */
#endif
#elif defined(IPOD_ARCH) && (MODEL_NUMBER==5)   /* iPOD Video */
# define CURRENT_NORMAL     50  /* 8h out of 400mAh battery (30GB) or 11h out of 600mAh (60GB) from IpodRuntime */
# define CURRENT_BACKLIGHT  20  /* estimation calculated from IpodRuntime measurement */
#if defined(HAVE_RECORDING)
# define CURRENT_RECORD     35  /* FIXME: this needs adjusting */
#endif
#elif defined(SANSA_E200)    /* Sandisk players */
# define CURRENT_NORMAL     50  /* Toni's measurements in spring 2007 suggests 50 ma during normal operation */
# define CURRENT_BACKLIGHT  20  /* seems like a reasonible value for now */
# define CURRENT_RECORD     35  /* FIXME: this needs adjusting */
#else /* Not iriver H1x0, H3x0, nor Archos Ondio, nor iPODVideo, nor Sansas */
# define CURRENT_NORMAL    145  /* usual current in mA when using the AJB including some disk/backlight/... activity */
# define CURRENT_BACKLIGHT  30  /* additional current when backlight always on */
#if defined(HAVE_RECORDING)
# define CURRENT_RECORD     35  /* FIXME: this needs adjusting */
#endif
#endif /* Not Archos Ondio */
#define CURRENT_USB        500  /* usual current in mA in USB mode */
#ifdef HAVE_REMOTE_LCD
# define CURRENT_REMOTE      8  /* add. current when H100-remote connected */
#endif /* HAVE_MMC */

# define CURRENT_MIN_CHG    70  /* minimum charge current */
# define MIN_CHG_V        8500  /* at 8.5v charger voltage get CURRENT_MIN_CHG */
# ifdef IRIVER_H300_SERIES
#  define CURRENT_MAX_CHG  650  /* maximum charging current */
# else
#  define CURRENT_MAX_CHG  350  /* maximum charging current */
# endif
# define MAX_CHG_V       10250  /* anything over 10.25v gives CURRENT_MAX_CHG */
#endif /* not HAVE_MMC */

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

/* read unfiltered battery info */
void battery_read_info(int *voltage, int *level);

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void);

void set_poweroff_timeout(int timeout);
void set_battery_capacity(int capacity); /* set local battery capacity value */
void set_battery_type(int type);         /* set local battery type */

void set_sleep_timer(int seconds);
int get_sleep_timer(void);
void set_car_adapter_mode(bool setting);
void reset_poweroff_timer(void);
void cancel_shutdown(void);
void shutdown_hw(void);
void sys_poweroff(void);

#endif
