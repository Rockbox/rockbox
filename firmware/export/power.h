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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _POWER_H_
#define _POWER_H_

#if CONFIG_CHARGING == CHARGING_CONTROL
extern bool charger_enabled;
void charger_enable(bool on);
#endif

#if CONFIG_CHARGING
bool charger_inserted(void);
#endif

void power_off(void);
void ide_power_enable(bool on);

# if CONFIG_CHARGING == CHARGING_MONITOR
bool charging_state(void);
# endif

#ifndef SIMULATOR

void power_init(void);

bool ide_powered(void);
#endif

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on);
bool spdif_powered(void);
#endif

#if CONFIG_TUNER
bool tuner_power(bool status);
#endif

#endif
