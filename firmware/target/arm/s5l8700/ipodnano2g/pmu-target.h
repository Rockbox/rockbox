/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Michael Sparmann
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

#ifndef __PMU_NANO2G_H__
#define __PMU_NANO2G_H__

#include "config.h"

unsigned char pmu_read(int address);
void pmu_write(int address, unsigned char val);
void pmu_read_multiple(int address, int count, unsigned char* buffer);
void pmu_write_multiple(int address, int count, unsigned char* buffer);
int pmu_read_battery_voltage(void);
int pmu_read_battery_current(void);
void pmu_init(void);
void pmu_switch_power(int gate, int onoff);

#endif
