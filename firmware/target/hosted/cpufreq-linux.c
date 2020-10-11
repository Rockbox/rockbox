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
 ****************************************************************************/


#include <stdio.h>
#include <string.h>

#include "config.h"
#include "cpuinfo-linux.h"
#include "debug.h"

#include "cpufreq-linux.h"


static FILE* open_read(const char* file_name)
{
    FILE *f = fopen(file_name, "re");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for reading.", __func__, file_name);
    }

    return f;
}


void cpufreq_available_governors(char* governors, int governors_size, int cpu)
{
    if(governors_size <= 0)
    {
        DEBUGF("ERROR %s: Invalid governors_size: %d.", __func__, governors_size);
        return;
    }

    if(cpu < 0)
    {
        DEBUGF("ERROR %s: Invalid cpu: %d.", __func__, cpu);
        return;
    }

    char available_governors_interface[70];

    snprintf(available_governors_interface,
             sizeof(available_governors_interface),
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_governors",
             cpu & 0xFFFF);

    FILE *f = open_read(available_governors_interface);
    if(f == NULL)
    {
        return;
    }

    if(fgets(governors, governors_size, f) == NULL)
    {
        DEBUGF("ERROR %s: Read failed for %s.", __func__, available_governors_interface);
        fclose(f);
        return;
    }

    DEBUGF("DEBUG %s: Available governors for cpu %d: %s.", __func__, cpu, governors);

    fclose(f);
}


static FILE* open_write(const char* file_name)
{
    FILE *f = fopen(file_name, "we");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for writing.", __func__, file_name);
    }

    return f;
}


static void set_governor_for_cpu(const char* governor, int cpu)
{
    DEBUGF("DEBUG %s: governor: %s, cpu: %d.", __func__, governor, cpu);

    char scaling_governor_interface[64];

    snprintf(scaling_governor_interface,
             sizeof(scaling_governor_interface),
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor",
             cpu);

    FILE *f = open_write(scaling_governor_interface);
    if(f == NULL)
    {
        return;
    }

    if(fprintf(f, "%s", governor) < 1)
    {
        DEBUGF("ERROR %s: Write failed for %s.", __func__, scaling_governor_interface);
    }

    fclose(f);
}


void cpufreq_set_governor(const char* governor, int cpu)
{
    if(governor == NULL)
    {
        DEBUGF("ERROR %s: Invalid governor.", __func__);
        return;
    }

    if(cpu < CPUFREQ_ALL_CPUS)
    {
        DEBUGF("ERROR %s: Invalid cpu: %d.", __func__, cpu);
        return;
    }

    DEBUGF("DEBUG %s: governor: %s, cpu: %d.", __func__, governor, cpu);

    if(cpu == CPUFREQ_ALL_CPUS)
    {
        int max_cpu = cpucount_linux();

        for(int count = 0; count < max_cpu; ++count)
        {
            set_governor_for_cpu(governor, count);
        }
    }
    else
    {
        set_governor_for_cpu(governor, cpu);
    }
}
