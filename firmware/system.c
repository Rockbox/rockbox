/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#include "config.h"
#include "system.h"
#include <stdio.h>
#include "kernel.h"
#include "thread.h"
#include "string.h"

#ifndef SIMULATOR
long cpu_frequency SHAREDBSS_ATTR = CPU_FREQ;
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static int boost_counter SHAREDBSS_ATTR = 0;
static bool cpu_idle SHAREDBSS_ATTR = false;
#if NUM_CORES > 1
static struct corelock boostctrl_cl SHAREDBSS_ATTR;
void cpu_boost_init(void)
{
    corelock_init(&boostctrl_cl);
}
#endif

int get_cpu_boost_counter(void)
{
    return boost_counter;
}
#ifdef CPU_BOOST_LOGGING
#define MAX_BOOST_LOG 64
static char cpu_boost_calls[MAX_BOOST_LOG][MAX_PATH];
static int cpu_boost_first = 0;
static int cpu_boost_calls_count = 0;
static int cpu_boost_track_message = 0;
int cpu_boost_log_getcount(void)
{
    return cpu_boost_calls_count;
}
char * cpu_boost_log_getlog_first(void)
{
    char *first;
    corelock_lock(&boostctrl_cl);

    first = NULL;

    if (cpu_boost_calls_count)
    {
        cpu_boost_track_message = 1;
        first = cpu_boost_calls[cpu_boost_first];
    }

    corelock_unlock(&boostctrl_cl);
    return first;
}

char * cpu_boost_log_getlog_next(void)
{
    int message;
    char *next;

    corelock_lock(&boostctrl_cl);

    message = (cpu_boost_track_message+cpu_boost_first)%MAX_BOOST_LOG;
    next = NULL;

    if (cpu_boost_track_message < cpu_boost_calls_count)
    {
        cpu_boost_track_message++;
        next = cpu_boost_calls[message];
    }

    corelock_unlock(&boostctrl_cl);
    return next;
}

void cpu_boost_(bool on_off, char* location, int line)
{
    corelock_lock(&boostctrl_cl);

    if (cpu_boost_calls_count == MAX_BOOST_LOG)
    {
        cpu_boost_first = (cpu_boost_first+1)%MAX_BOOST_LOG;
        cpu_boost_calls_count--;
        if (cpu_boost_calls_count < 0)
            cpu_boost_calls_count = 0;
    }
    if (cpu_boost_calls_count < MAX_BOOST_LOG)
    {
        int message = (cpu_boost_first+cpu_boost_calls_count)%MAX_BOOST_LOG;
        snprintf(cpu_boost_calls[message], MAX_PATH,
                    "%c %s:%d",on_off==true?'B':'U',location,line);
        cpu_boost_calls_count++;
    }
#else
void cpu_boost(bool on_off)
{
    corelock_lock(&boostctrl_cl);
#endif /* CPU_BOOST_LOGGING */
    if(on_off)
    {
        /* Boost the frequency if not already boosted */
        if(++boost_counter == 1)
            set_cpu_frequency(CPUFREQ_MAX);
    }
    else
    {
        /* Lower the frequency if the counter reaches 0 */
        if(--boost_counter <= 0)
        {
            if(cpu_idle)
                set_cpu_frequency(CPUFREQ_DEFAULT);
            else
                set_cpu_frequency(CPUFREQ_NORMAL);

            /* Safety measure */
            if (boost_counter < 0)
            {
                boost_counter = 0;
            }
        }
    }

    corelock_unlock(&boostctrl_cl);
}

void cpu_idle_mode(bool on_off)
{
    corelock_lock(&boostctrl_cl);

    cpu_idle = on_off;

    /* We need to adjust the frequency immediately if the CPU
       isn't boosted */
    if(boost_counter == 0)
    {
        if(cpu_idle)
            set_cpu_frequency(CPUFREQ_DEFAULT);
        else
            set_cpu_frequency(CPUFREQ_NORMAL);
    }

    corelock_unlock(&boostctrl_cl);
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */


#ifdef HAVE_FLASHED_ROCKBOX
static bool detect_flash_header(uint8_t *addr)
{
#ifndef BOOTLOADER
    int oldmode = system_memory_guard(MEMGUARD_NONE);
#endif
    struct flash_header hdr;
    memcpy(&hdr, addr, sizeof(struct flash_header));
#ifndef BOOTLOADER
    system_memory_guard(oldmode);
#endif
    return hdr.magic == FLASH_MAGIC;
}
#endif

bool detect_flashed_romimage(void)
{
#ifdef HAVE_FLASHED_ROCKBOX
    return detect_flash_header((uint8_t *)FLASH_ROMIMAGE_ENTRY);
#else
    return false;
#endif /* HAVE_FLASHED_ROCKBOX */
}

bool detect_flashed_ramimage(void)
{
#ifdef HAVE_FLASHED_ROCKBOX
    return detect_flash_header((uint8_t *)FLASH_RAMIMAGE_ENTRY);
#else
    return false;
#endif /* HAVE_FLASHED_ROCKBOX */
}

bool detect_original_firmware(void)
{
    return !(detect_flashed_ramimage() || detect_flashed_romimage());
}

