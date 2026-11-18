/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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
#include "sys_storage_psp.h"
#include "kernel.h"
#include "thread-console.h"
#include "system-console.h"
#include "button-console.h"
#include "lcd-bitmap.h"
#include "lcd.h"
#include "panic.h"
#include "powermgmt.h"
#include "debug.h"
#include "disk.h"

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

/* filesystem: file_internal_mrsw is defined and set up by common/file_internal.c */

const char      *audiodev = NULL;

void psp_sys_quit(void)
{
    sys_poweroff();
}

void sim_do_exit(void);

void power_off(void)
{
    /* Every Rockbox thread lives in this OS thread: none to join */
    sim_do_exit();
}

void sim_do_exit()
{
    sys_storage_reattach();     /* give the card back to Sony's driver */
    sim_kernel_shutdown();
    sys_timer_quit();
    exit(0);
}

uintptr_t *stackbegin;
uintptr_t *stackend;

/* HOME-button exit.
 *
 * Without an exit callback, "Quit" from the HOME menu ends Rockbox without
 * its shutdown: unsaved settings are lost, a FAT update could be left half
 * done and ms0: stays detached until the system software reboots.
 *
 * The callback runs on its own OS thread, which is not a Rockbox thread, so
 * it only raises a flag. The button driver polls it from the tick
 * (psp_exit_poll) and calls sys_poweroff() there, as Rockbox's button code
 * itself does for a long power-button press. The normal shutdown then ends
 * in sim_do_exit(), which commits and reattaches ms0:. */

static volatile int exit_requested;     /* 0 no, 1 requested, 2 handled */

static int exit_callback(int arg1, int arg2, void *common)
{
    (void)arg1; (void)arg2; (void)common;
    if (!exit_requested)
        exit_requested = 1;
    /* Rockbox ends the process itself (sceKernelExitGame) when its
       shutdown is done; nothing more to do here */
    return 0;
}

static int callback_thread(SceSize args, void *argp)
{
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("rb_exit", exit_callback, NULL);
    if (cbid >= 0)
        sceKernelRegisterExitCallback(cbid);
    else
        DEBUGF("exit callback: 0x%x\n", cbid);
    sceKernelSleepThreadCB();               /* callbacks run in here */
    return 0;
}

/* from the button driver, in tick context */
void psp_exit_poll(void)
{
    if (exit_requested == 1) {
        exit_requested = 2;
        sys_poweroff();
    }
}

static void exit_init(void)
{
    SceUID t = sceKernelCreateThread("rb_exitcb", callback_thread, 0x11,
                                     4 * 1024, PSP_THREAD_ATTR_USER, NULL);
    if (t >= 0)
        sceKernelStartThread(t, 0, NULL);
    else
        DEBUGF("exit callback thread: 0x%x\n", t);
}

void system_init(void)
{
    /* fake stack, OS manages size (and growth) */
    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

    atexit(sceKernelExitGame);
    exit_init();            /* HOME exit through the normal shutdown */

    int result = sceKernelSetCompiledSdkVersion(0x06060010);
    if (result < 0) {
    DEBUGF("sceKernelSetCompiledSdkVersion: error 0x%x\n", result);
    return;
    }

#ifdef DEBUG
    SceKernelThreadInfo status;
    status.size = sizeof(SceKernelThreadInfo);
    if (sceKernelReferThreadStatus(sceKernelGetThreadId(), &status) == 0) {
        int priority = status.currentPriority;
        DEBUGF("main thread priority: %d\n", priority);
    }
#endif

    cpu_frequency = scePowerGetCpuClockFrequencyInt();
    DEBUGF("cpu running at %ld Mhz\n", cpu_frequency);

#ifdef DEBUG
    SceSize am = sceKernelTotalFreeMemSize();
    DEBUGF("\nMemory available for plugins: %d bytes (%dKB or %dMB)\n",
        am, am / 1024, am / 1024 / 1024 );
#endif

    sys_timer_init();
    sys_init_kernel_hook();

    /* filesystem: filesystem_init() in apps/main.c */
}


void system_reboot(void)
{
    /* panic or exception: give ms0: back before waiting for good, so the
       card is in Sony's hands when the user quits from the HOME menu */
    sys_storage_reattach();
    /* wait here until "Quit" from the HOME menu; the tick that runs
       psp_exit_poll() may be stopped, so check the request here */
    while (!exit_requested)
        sys_delay(100);
    sceKernelExitGame();
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

