/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/* this includes a couple of 3ds headers */
#include "bfile.h"
#include "sys_timer.h"

#include <3ds/types.h>
#include <3ds/allocator/linear.h>
#include <3ds/services/cfgu.h>
#include <3ds/env.h>

#include "system.h"
#include "kernel.h"
#include "thread-console.h"
#include "system-console.h"
#include "button-console.h"
#include "lcd-bitmap.h"
#include "panic.h"
#include "debug.h"

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
    sim_do_exit();
}

void power_off(void)
{
    /* shut down Rockbox kernel threads */
    struct thread_entry* t = sim_thread_unlock();
    sim_thread_shutdown();

    /* lock again before entering the scheduler */
    sim_thread_lock(t);
    sim_do_exit();
}

void sim_do_exit()
{
    sim_kernel_shutdown();
    sys_timer_quit();
    mcuhwc_close();
    cfguExit();
    exit(EXIT_SUCCESS);
}

int test_stream_exists_internal(const char *path);
bool dump_dsp_firmware(void)
{
    DEBUGF("dump_dsp_firmware: fetching dsp component.\n");
    bool failed = true;
    Handle rsrc = envGetHandle("hb:ndsp");
    if (!rsrc)
    {
        DEBUGF("dump_dsp_firmware: need to run using *hax 2.0+\n");
        return false;
    }
    else do
    {
        Result rc;
        u32 len;
        void* bin;
        extern u32 fake_heap_end;
        char* filename = "sdmc:/3ds/dspfirm.cdc";

        u32 mapAddr = (fake_heap_end + 0xFFF) &~ 0xFFF;
        rc = svcMapMemoryBlock(rsrc, mapAddr, 0x3, 0x3);
        if (R_FAILED(rc)) break;

        len = *(u32*)(mapAddr + 0x104);
        bin = malloc(len);
        if (bin)
            memcpy(bin, (void*)mapAddr, len);
        svcUnmapMemoryBlock(rsrc, mapAddr);
        if (!bin) break;

        FILE* file = fopen(filename, "wb");
        if (!file) break;
        fwrite(bin, 1, len, file);
        fclose(file);

        failed = false;
        DEBUGF("dump_dsp_firmware: done\n");
        free(bin);
    } while (0);

    if (rsrc && failed) {
        DEBUGF("dump_dsp_firmware: unknown error.\n");
    }

    return true;
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
        DEBUGF("warning, main_thread_priority = 0x%lx\n", main_thread_priority);
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

    /* dsp firmware check */
    if (test_stream_exists_internal("/3ds/dspfirm.cdc") < 0) {
        DEBUGF("system_init: /3ds/dspfirm.cdc not found.\n");
        dump_dsp_firmware();
    }
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

