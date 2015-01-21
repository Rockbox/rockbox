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


/*
    Get the available governors for cpu.
    governors Preallocated string for the available governors. On succes this will look like
              "performance ondemand powersave", each word beeing a governor that can be used with
              cpufreq_set_governor.
    governors_size Size of governors.
    cpu The cpu, starting with 0, for which to get the available governors. CPUFREQ_ALL_CPUS is not
        supported, you must supply a cpu. See cpucount_linux in cpuinfo-linux.h.
*/
void cpufreq_available_governors(char* governors, int governors_size, int cpu);


/*
    Set the cpufreq governor for cpu.
    governor The governor to set. This must be one of the available governors returned by
             cpufreq_available_governors.
    cpu The cpu, starting with 0, for which to set the governor. CPUFREQ_ALL_CPUS to set
        the governor for all cpus. See cpucount_linux in cpuinfo-linux.h.
*/
void cpufreq_set_governor(const char* governor, int cpu);


static const int CPUFREQ_ALL_CPUS = -1;


#endif
