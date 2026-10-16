/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/socket.h>
#include "system.h"
#include "kernel.h"
#include "thread-console.h"
#include "system-console.h"
#include "button-console.h"
#include "lcd-bitmap.h"
#include "panic.h"
#include "debug.h"

#include <psploadexec.h>
#include <pspmoduleinfo.h>
#include <pspthreadman.h>
#include <pspsysmem.h>
#include <psppower.h>

#include "sys_timer.h"
#include "sys_kernel_hook.h"

PSP_MODULE_INFO("rockbox", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);
PSP_MAIN_THREAD_PRIORITY(24);
PSP_HEAP_SIZE_KB(8 * 1024);

/* filesystem */
#include "bfile.h"
sync_RWMutex file_internal_mrsw;

const char      *audiodev = NULL;

void psp_sys_quit(void)
{
    sys_poweroff();
}

void power_off(void)
{
    /* since sim_thread_shutdown() grabs the mutex we need to let it free,
     * otherwise sys_wait_thread will deadlock */
    struct thread_entry* t = sim_thread_unlock();
    sim_thread_shutdown();

    /* lock again before entering the scheduler */
    sim_thread_lock(t);
    /* sim_thread_shutdown() will cause sim_do_exit() to be called via longjmp,
     * but only if we let the sdl thread scheduler exit the other threads */
    while(1) yield();
}

void sim_do_exit()
{
    sim_kernel_shutdown();
    sys_timer_quit();
    exit(0);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    /* fake stack, OS manages size (and growth) */
    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

    atexit(sceKernelExitGame);

    int result = sceKernelSetCompiledSdkVersion(0x06060010);
    if (result < 0) {
    DEBUGF("sceKernelSetCompiledSdkVersion: error 0x%x\n", result);
    return;
    }

    SceKernelThreadInfo status;
    status.size = sizeof(SceKernelThreadInfo);
    if (sceKernelReferThreadStatus(sceKernelGetThreadId(), &status) == 0) {
        int priority = status.currentPriority;
        DEBUGF("main thread priority: %d\n", priority);
    }

    cpu_frequency = scePowerGetCpuClockFrequencyInt();
    DEBUGF("cpu running at %ld Mhz\n", cpu_frequency);

    SceSize am = sceKernelTotalFreeMemSize();
    DEBUGF("\nMemory available for plugins: %d bytes (%dKB or %dMB)\n",
        am, am / 1024, am / 1024 / 1024 );

    sys_timer_init();
    sys_init_kernel_hook();

    /* filesystem */
    sync_RWMutexInit(&file_internal_mrsw);
}


void system_reboot(void)
{
    sim_thread_exception_wait();
}

void system_exception_wait(void)
{
    system_reboot();
}

int hostfs_init(void)
{
    /* stub */
    return 0;
}

#ifdef HAVE_STORAGE_FLUSH
int hostfs_flush(void)
{
    sceIoSync("ms0:", 0);
    return 0;
}
#endif /* HAVE_STORAGE_FLUSH */

void set_cpu_frequency(long frequency)
{
    if (cpu_frequency == frequency)
        return;

    /* DEBUGF("set_cpu_frequency: %ld Mhz\n", frequency); */
   	scePowerSetClockFrequency(frequency, frequency, frequency / 2);
    cpu_frequency = frequency;
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

