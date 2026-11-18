/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Mauricio G.
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

/* Memory Stick ownership for Rockbox's own file stack.
 *
 * Rockbox's own FAT driver works on the raw card (psp/ms-psp.c).
 * Sony's driver (fatms) keeps its own caches of the same filesystem through
 * ms0:, so the two must not both use the card. At storage init Rockbox
 * flushes and detaches ms0: and becomes the only user of the card; ms0: is
 * reattached at exit.
 *
 * The OS module loader still needs ms0: for codecs and plugins (loading
 * from a memory buffer is refused in user mode, write-probe P2). For each
 * load Rockbox opens a short window: it holds its file-system writer lock
 * (no Rockbox file operation can run), commits its disk cache, reattaches
 * ms0: (fatms mounts fresh and so reads the card exactly as Rockbox left
 * it, write-probe P6), lets the loader read the module and detaches again.
 * fatms only reads in the window. Measured on PHAT: reattach 1.1 ms,
 * detach 60 us, a whole codec load window 19 ms, the same as a load today
 * (write-probe P1-P5). */

#include <stdlib.h>
#include <stdio.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "file_internal.h"
#include "disk_cache.h"
#include "debug.h"
#include "sys_storage_psp.h"

static volatile int ms0_detached;

static int ms0_attach(void)
{
    int rc = sceIoAssign("ms0:", "msstor0p1:", "fatms0:", IOASSIGN_RDWR, NULL, 0);
    if (rc < 0) {
        DEBUGF("ms0: assign failed 0x%x\n", rc);
    }
    return rc;
}

static int ms0_unassign(void)
{
    int rc = sceIoUnassign("ms0:");
    if (rc < 0) {
        DEBUGF("ms0: unassign failed 0x%x\n", rc);
    }
    return rc;
}

/* at exit: called by sim_do_exit (system-psp.c); the atexit registration is
   only a fallback (the PSP exit path did not run atexit handlers in tests).
   Only the first call does anything. */
void sys_storage_reattach(void)
{
    if (!ms0_detached)
        return;
    ms0_detached = 0;
    int rc = ms0_attach();
    SceUID d = sceIoDopen("ms0:/");         /* is ms0: usable again? */
    if (d >= 0)
        sceIoDclose(d);
    DEBUGF("ms0: reattached at exit: assign=0x%x dopen=0x%x\n", rc, d);
    (void)rc;
    fflush(stdout);         /* the process ends right after this */
}

void sys_storage_detach(void)
{
    if (ms0_detached)
        return;
    sceIoSync("ms0:", 0);                   /* fatms writes back its cache */
    if (ms0_unassign() < 0)
        return;
    ms0_detached = 1;
    atexit(sys_storage_reattach);
}

void sys_storage_window_open(void)
{
    if (!ms0_detached)
        return;
    file_internal_lock_WRITER();            /* no Rockbox file operation now */
    dc_lock_cache();
    dc_commit_all(IF_MV(0));                /* the card as Rockbox sees it */
    dc_unlock_cache();
    ms0_attach();
}

void sys_storage_window_close(void)
{
    if (!ms0_detached)
        return;
    ms0_unassign();
    file_internal_unlock_WRITER();
}
