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

#define POWER_HISTORY_LEN 2*60   /* 2 hours of samples, one per minute */
#define POWER_AVG         3      /* how many samples to take for each measurement */

#define CHARGE_END_NEGD   6      /* stop when N minutes have passed with
                                  * avg delta being < -0.3 V */
#define CHARGE_END_ZEROD  30     /* stop when N minutes have passed with
                                  * avg delta being < 0.005 V */

#ifdef HAVE_CHARGE_CTRL
#define POWER_MESSAGE_LEN 32     /* power thread status message */
#define CHARGE_MAX_TIME   6*60   /* minutes: maximum charging time */
#define CHARGE_MIN_TIME   10     /* minutes: minimum charging time */
#define CHARGE_RESTART_HI 90     /* %: when to restart charging in 'charge' mode */
#define CHARGE_RESTART_LO 10     /* %: when to restart charging in 'discharge' mode */

extern char power_message[POWER_MESSAGE_LEN];
extern char charge_restart_level;
#endif /* HAVE_CHARGE_CTRL */

extern unsigned short power_history[POWER_HISTORY_LEN];

void power_init(void);

#endif

#endif
