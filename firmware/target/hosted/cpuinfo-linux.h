/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2011 Thomas Martitz
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


#ifndef __CPUINFO_LINUX_H__
#define __CPUINFO_LINUX_H__

struct cpuusage {
    long usage; /* in hundredth percent */
    long utime; /* in clock ticks */
    long stime; /* in clock ticks */
    long rtime; /* in clock ticks */
    long hz;    /* how clock ticks per second */
};

struct time_state {
    long frequency;
    long time;
};

int cpuusage_linux(struct cpuusage* u);
int cpufrequency_linux(int cpu);
int scalingfrequency_linux(int cpu);
int cpustatetimes_linux(int cpu, struct time_state* data, int max_elements);
int cpucount_linux(void);

#endif /* __CPUINFO_LINUX_H__ */
