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
int pmu_write(int address, unsigned char val);
int pmu_read_multiple(int address, int count, unsigned char* buffer);
int pmu_write_multiple(int address, int count, unsigned char* buffer);
int pmu_read_adc(unsigned int adc);
int pmu_read_battery_voltage(void);
int pmu_read_battery_current(void);
void pmu_init(void);
void pmu_ldo_on_in_standby(unsigned int ldo, int onoff);
void pmu_ldo_set_voltage(unsigned int ldo, unsigned char voltage);
void pmu_ldo_power_on(unsigned int ldo);
void pmu_ldo_power_off(unsigned int ldo);
void pmu_set_wake_condition(unsigned char condition);
void pmu_enter_standby(void);
void pmu_read_rtc(unsigned char* buffer);
void pmu_write_rtc(unsigned char* buffer);

#endif
