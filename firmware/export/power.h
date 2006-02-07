/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _POWER_H_
#define _POWER_H_

#ifdef HAVE_CHARGE_CTRL
extern bool charger_enabled;
void charger_enable(bool on);
#endif

#ifdef HAVE_CHARGING
bool charger_inserted(void);
#endif

void power_off(void);
void ide_power_enable(bool on);

#ifndef SIMULATOR

void power_init(void);

# ifdef HAVE_CHARGE_STATE
bool charging_state(void);
# endif

bool ide_powered(void);
#endif

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on);
#endif

#ifdef CONFIG_TUNER
extern bool radio_power(bool status);
extern bool radio_powered(void);
#endif

#endif
