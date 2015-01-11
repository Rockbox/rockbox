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

#include "config.h"
#include "cpuinfo-linux.h"
#include "debug.h"

#include "cpufreq-linux.h"


static FILE* open_write(const char* file_name)
{
    FILE *f = fopen(file_name, "w");
    if(f == NULL)
    {
        DEBUGF("ERROR %s: Can not open %s for writing.", __func__, file_name);
    }

    return f;
}


static void set_governor_for_cpu(const char* governor, int cpu)
{
    if(governor == NULL)
    {
        DEBUGF("DEBUG %s: Invalid governor.", __func__);
        return;
    }

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
        DEBUGF("DEBUG %s: Invalid governor.", __func__);
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
