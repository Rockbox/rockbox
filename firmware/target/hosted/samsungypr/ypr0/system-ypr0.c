/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2011-2013 by Lorenzo Miori
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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "system.h"
#include "panic.h"
#include "debug.h"

#if defined(HAVE_SDL_AUDIO) || defined(HAVE_SDL_THREADS) || defined(HAVE_SDL)
#include <SDL.h>
#endif

#include "ascodec.h"
#include "gpio-target.h"

void power_off(void)
{
    /* Something that we need to do before exit on our platform YPR0 */
    ascodec_close();
    gpio_close();
    exit(EXIT_SUCCESS);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    int *s;
    /* fake stack, OS manages size (and growth) */
    stackbegin = stackend = (uintptr_t*)&s;

#if defined(HAVE_SDL_AUDIO) || defined(HAVE_SDL_THREADS) || defined(HAVE_SDL)
    SDL_Init(0); /* need this if using any SDL subsystem */
#endif

    /* Here begins our platform specific initilization for various things */
    ascodec_init();
    gpio_init();
}


void system_reboot(void)
{
    power_off();
}

void system_exception_wait(void)
{
    system_reboot();
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#include <stdio.h>
#include "file.h"
/* This is the Linux Kernel CPU governor... */
static void set_cpu_freq(int speed)
{
    char temp[10];
    int cpu_dev;
    cpu_dev = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", O_WRONLY);
    if (cpu_dev < 0)
        return;
    write(cpu_dev, temp, sprintf(temp, "%d", speed) + 1);
    close(cpu_dev);
}

void set_cpu_frequency(long frequency)
{
    switch (frequency)
    {
        case CPUFREQ_MAX:
            set_cpu_freq(532000);
            cpu_frequency = CPUFREQ_MAX;
            break;
        case CPUFREQ_NORMAL:
            set_cpu_freq(400000);
            cpu_frequency = CPUFREQ_NORMAL;
            break;
        default:
            set_cpu_freq(200000);
            cpu_frequency = CPUFREQ_DEFAULT;
            break;
    }
}
#endif
