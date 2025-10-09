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
#include "system.h"
#include "kernel.h"
#include "thread-ctru.h"
#include "system-ctru.h"
#include "button-ctru.h"
#include "lcd-bitmap.h"
#include "panic.h"
#include "debug.h"

#include <3ds/types.h>
#include <3ds/allocator/linear.h>
#include <3ds/services/cfgu.h>
#include "bfile.h"

const char      *audiodev = NULL;

#ifdef DEBUG
bool debug_audio = false;
#endif

/* default main thread priority, low */
s32 main_thread_priority = 0x30;
bool is_n3ds = false;

/* filesystem */
sync_RWMutex file_internal_mrsw;
FS_Archive sdmcArchive;

void ctru_sys_quit(void)
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
    /* TODO: quit_everything() */
    exit(EXIT_SUCCESS);
}

uintptr_t *stackbegin;
uintptr_t *stackend;
void system_init(void)
{
    /* fake stack, OS manages size (and growth) */
    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

    /* disable sleep mode when lid is closed */
    aptSetSleepAllowed(false);

    sys_console_init();
    sys_timer_init();
    
    svcGetThreadPriority(&main_thread_priority, CUR_THREAD_HANDLE);
    if (main_thread_priority != 0x30) {
        DEBUGF("warning, main_thread_priority = 0x%x\n", main_thread_priority);
    }
    
    /* check for New 3DS model */
    s64 dummyInfo;
    is_n3ds = svcGetSystemInfo(&dummyInfo, 0x10001, 0) == 0;

    /* filesystem */
    sync_RWMutexInit(&file_internal_mrsw);
    Result res = FSUSER_OpenArchive(&sdmcArchive,
                                    ARCHIVE_SDMC,
                                    fsMakePath(PATH_ASCII, ""));
    if (R_FAILED(res)) {
        DEBUGF("FSUSER_OpenArchive failed\n");
        exit(-1);
    }
    
    mcuhwc_init();
    cfguInit();
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
    /* romfsInit(); */
    return 0;
}

#ifdef HAVE_STORAGE_FLUSH
int hostfs_flush(void)
{
#ifdef __unix__
    sync();
#endif
    return 0;
}
#endif /* HAVE_STORAGE_FLUSH */

