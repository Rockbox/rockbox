/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 by Udo Schläpfer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 *
 * Interface for the Linux CPUFreq kernel modul.
 * See https://www.kernel.org/doc/Documentation/cpu-freq/
 *
 ****************************************************************************/


#ifndef __CPUFREQ_LINUX_H__
#define __CPUFREQ_LINUX_H__


#include <stdbool.h>


/*!
    \brief Set the cpufreq governor for cpu.
    \param governor The governor to set. This must be one of the governors listed in
                    /sys/devices/system/cpu/cpuX/cpufreq/scaling_available_governors, where
                    X is the cpu.
    \param cpu The cpu, starting with 0, for which to set the governor. CPUFREQ_ALL_CPUS to set
               the governor for all cpus. See cpucount_linux in cpuinfo-linux.h.
*/
void cpufreq_set_governor(const char* governor, int cpu);


static const int CPUFREQ_ALL_CPUS = -1;


#endif
