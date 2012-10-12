/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id: system-sdl.c 29925 2011-05-25 20:11:03Z thomasjfox $
 *
 * Copyright (C) 2006 by Daniel Everton <dan@iocaine.org>
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
#include "kernel.h"

#if defined(HAVE_SDL_AUDIO) || defined(HAVE_SDL_THREADS) || defined(HAVE_SDL)
#include <SDL.h>
#endif

#include <sys/mount.h>
#include "ascodec.h"
#include "gpio_ypr0.h"
#include "backlight.h"

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
    /* Some ROMs could mount SD card without the sync and/or other proper options
     * Then unmount and remount microSD, even if it is useless, pretty light operation!
     */
    umount_microSD();
    mount_microSD();
}


void system_reboot(void)
{
    power_off();
}

void system_exception_wait(void)
{
    system_reboot();
}

/* MicroSD card removal / insertion management */

static int microSD_stack[DEFAULT_STACK_SIZE];
bool microSD_state = false;

bool mount_microSD(void) {
    int ret = mount("/dev/mmcblk0p1", "/mnt/mmc", "vfat", MS_MGC_VAL | MS_SYNCHRONOUS | MS_RELATIME, "iocharset=utf8");
    if (ret < 0) {
        return false;
    }
    return true;
}
bool umount_microSD(void) {
    int ret = umount2("/mnt/mmc", MNT_FORCE);
    if (ret < 0) {
        return false;
    }
    return true;
}

bool microSD_inserted(void) {
    /* Active LOW */
    return (!gpio_control(DEV_CTRL_GPIO_IS_HIGH, GPIO_SD_SENSE, 0, 0));
}

static inline void microSD_check(void)
{
    bool microsd_init = false;
    if (microSD_inserted()) {
        if (!microSD_state) {
            queue_broadcast(SYS_HOTSWAP_INSERTED, 0);
            backlight_on();
            /* Wait for the kernel to load microSD device */
            sleep(HZ);
            microsd_init = mount_microSD();
            microSD_state = true;
        }
    }
    else {
        if (microSD_state) {
            queue_broadcast(SYS_HOTSWAP_EXTRACTED, 0);
            backlight_on();
            microsd_init = umount_microSD();
            microSD_state = false;
        }
    }
    if (microsd_init)  
        queue_broadcast(SYS_FS_CHANGED, 0);
    sleep(HZ);
}

static void NORETURN_ATTR microSD_thread(void)
{
    while (1) {
        microSD_check();
    }
}

void sdsense_init(void) {
    /* Setup GPIO pin for microSD sense, copied from OF */
    gpio_control(DEV_CTRL_GPIO_SET_MUX, GPIO_SD_SENSE, CONFIG_DEFAULT, 0);
    gpio_control(DEV_CTRL_GPIO_SET_INPUT, GPIO_SD_SENSE, CONFIG_DEFAULT, 0);
    
    microSD_state = microSD_inserted();
    create_thread(microSD_thread, microSD_stack, sizeof(microSD_stack), 0, "microSD"
        IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
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
