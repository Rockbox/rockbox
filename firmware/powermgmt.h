/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Heikki Hannikainen
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

#ifndef SIMULATOR

#define BATTERY_LEVEL_SHUTDOWN   450 /* 4.5V */
#define BATTERY_LEVEL_EMPTY      465 /* 4.65V */
#define BATTERY_LEVEL_DANGEROUS  475 /* 4.75V */
#define BATTERY_LEVEL_FULL       585 /* 5.85V */

#define BATTERY_RANGE (BATTERY_LEVEL_FULL - BATTERY_LEVEL_EMPTY)

#define POWER_HISTORY_LEN 2*60   /* 2 hours of samples, one per minute */
#define POWER_AVG_N       4      /* how many samples to take for each measurement */
#define POWER_AVG_SLEEP   10     /* how long do we sleep between each measurement */

#define CHARGE_END_NEGD   6      /* stop when N minutes have passed with
                                  * avg delta being < -0.05 V */
#define CHARGE_END_ZEROD  50     /* stop when N minutes have passed with
                                  * avg delta being < 0.005 V */

#ifdef HAVE_CHARGE_CTRL
#define POWER_MESSAGE_LEN 32     /* power thread status message */
#define CHARGE_MAX_TIME   8*60   /* minutes: maximum charging time */
#define CHARGE_MIN_TIME   10     /* minutes: minimum charging time */
#define CHARGE_RESTART_HI 95     /* %: when to restart charging in 'charge' mode */
#define CHARGE_RESTART_LO 10     /* %: when to restart charging in 'discharge' mode */
#define CHARGE_PAUSE_LEN  60     /* how many minutes to pause between charging cycles */

extern char power_message[POWER_MESSAGE_LEN];
extern char charge_restart_level;

extern int powermgmt_last_cycle_startstop_min; /* how many minutes ago was the charging started or stopped? */
extern int powermgmt_last_cycle_level;         /* which level had the batteries at this time? */

extern int battery_lazyness[20]; /* how does the battery react when plugging in/out the charger */

#endif /* HAVE_CHARGE_CTRL */

#define BATTERY_CAPACITY 1800    /* battery capacity in mAh for runtime estimation */
#define CURRENT_NORMAL    145    /* usual current in mA when using the AJB including some disk/backlight/... activity */
#define CURRENT_BACKLIGHT  30    /* additional current when backlight is always on */
#define CURRENT_CHARGING  300    /* charging current */

extern unsigned short power_history[POWER_HISTORY_LEN];

/* Start up power management thread */
void power_init(void);

#endif /* SIMULATOR */

/* Returns battery level in percent */
int battery_level(void);
int battery_time(void); /* minutes */

/* Tells if the battery level is safe for disk writes */
bool battery_level_safe(void);

void set_poweroff_timeout(int timeout);

void set_sleep_timer(int seconds);
int get_sleep_timer(void);

#endif
