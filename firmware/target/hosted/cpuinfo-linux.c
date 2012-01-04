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


#include <sys/times.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "kernel.h"
#include "thread.h"
#include "cpuinfo-linux.h"
#include "gcc_extensions.h"

#undef open /* want the *real* open here, not sim_open or the like */
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
#include "cpu-features.h"
#define get_nprocs android_getCpuCount
#endif

#define NUM_SAMPLES 64
#define NUM_SAMPLES_MASK (NUM_SAMPLES-1)
#define SAMPLE_RATE  4

struct cputime_sample {
    struct tms sample;
    time_t time;
};

static struct cputime_sample samples[NUM_SAMPLES];
static int current_sample;
static int samples_taken;
static clock_t initial_time, hz;


static char cputime_thread_stack[DEFAULT_STACK_SIZE + 0x100];
static char cputime_thread_name[] = "cputime";
static int  cputime_threadid;
static void cputime_thread(void);

/* times() can return -1 early after boot */
static clock_t times_wrapper(struct tms *t)
{
    clock_t res = times(t);
    if (res == (clock_t)-1)
        res = time(NULL) * hz;

    return res;
}

static inline void thread_func(void)
{
    struct cputime_sample *s = &samples[current_sample++];
    s->time = times_wrapper(&s->sample);

    current_sample &= NUM_SAMPLES_MASK;
    if (samples_taken < NUM_SAMPLES) samples_taken++;

    sleep(HZ/SAMPLE_RATE);
}

static void NORETURN_ATTR cputime_thread(void)
{
    while(1)
        thread_func();
}

static void __attribute__((constructor)) get_initial_time(void)
{
    struct tms ign;
    hz = sysconf(_SC_CLK_TCK);
    initial_time = times_wrapper(&ign); /* dont pass NULL */;
}

int cpuusage_linux(struct cpuusage* u)
{
    if (UNLIKELY(!cputime_threadid))
    {
        /* collect some initial data and start the thread */
        thread_func();
        cputime_threadid = create_thread(cputime_thread, cputime_thread_stack,
                            sizeof(cputime_thread_stack), 0, cputime_thread_name
                            IF_PRIO(,PRIORITY_BACKGROUND) IF_COP(, CPU));
    }
    if (!u)
        return -1;

    clock_t total_cputime;
    clock_t diff_utime, diff_stime;
    time_t  diff_rtime;
    int latest_sample = ((current_sample == 0) ? NUM_SAMPLES : current_sample) - 1;
    int oldest_sample = (samples_taken < NUM_SAMPLES) ? 0 : current_sample;

    diff_utime = samples[latest_sample].sample.tms_utime - samples[oldest_sample].sample.tms_utime;
    diff_stime = samples[latest_sample].sample.tms_stime - samples[oldest_sample].sample.tms_stime;
    diff_rtime = samples[latest_sample].time - samples[oldest_sample].time;
    if (UNLIKELY(!diff_rtime))
        diff_rtime = 1;
    u->hz    = hz;

    u->utime = samples[latest_sample].sample.tms_utime;
    u->stime = samples[latest_sample].sample.tms_stime;
    u->rtime = samples[latest_sample].time - initial_time;

    total_cputime = diff_utime + diff_stime;
    total_cputime *= 100; /* pump up by 100 for hundredth */
    u->usage = total_cputime * 100 / diff_rtime;

    return 0;
}

int cpucount_linux(void)
{
    return get_nprocs();
}

int cpufrequency_linux(int cpu)
{
    char path[64];
    char temp[10];
    int cpu_dev, ret = -1;
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq", cpu);
    cpu_dev = open(path, O_RDONLY);
    if (cpu_dev < 0)
        return -1;
    if (read(cpu_dev, temp, sizeof(tmp)) >= 0)
        ret = atoi(temp);
    close(cpu_dev);
    return ret;
}

int scalingfrequency_linux(int cpu)
{
    char path[64];
    char temp[10];
    int cpu_dev, ret = -1;
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu);
    cpu_dev = open(path, O_RDONLY);
    if (cpu_dev < 0)
        return -1;
    if (read(cpu_dev, temp, sizeof(tmp)) >= 0)
        ret = atoi(temp);
    close(cpu_dev);
    return ret;
}

int cpustatetimes_linux(int cpu, struct time_state* data, int max_elements)
{
    int elements_left = max_elements, cpu_dev;
    char buf[256], path[64], *p;
    ssize_t read_size;
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/stats/time_in_state", cpu);
    cpu_dev = open(path, O_RDONLY);
    if (cpu_dev < 0)
        return -1;
    read_size = read(cpu_dev, buf, sizeof(buf));

    close(cpu_dev);

    p = buf;
    while(elements_left > 0 && (p-buf) < read_size)
    {
        data->frequency = atol(p);
        /* this loop breaks when it seems the space or line-feed,
         * so buf points to a number aftwards */
        while(isdigit(*p++));
        data->time = atol(p);
        /* now skip over to the next line */
        while(isdigit(*p++));
        data++;
        elements_left--;
    }

    return (max_elements - elements_left) ?: -1;
}
