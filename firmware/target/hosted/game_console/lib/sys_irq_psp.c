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

/* Interrupt support for the native scheduler with real interrupt masking.
 *
 * All Rockbox threads run in the main OS thread. The "interrupt handlers"
 * (tick, user timer, PCM callback) run in OS threads of higher priority.
 *
 *  - Rockbox disabling interrupts masks the CPU's interrupts
 *    (sceKernelCpuSuspendIntr, user-mode, 0.15 us measured). Nothing is
 *    dispatched while they are masked: no handler thread can start
 *    (measured: a timer-woken priority-16 thread ran 0 times in 3 ms).
 *  - While a handler runs, the Rockbox thread cannot run, because handler
 *    threads have higher priority. So a handler needs no lock against Rockbox
 *    threads, only against other handlers (an LwMutex).
 *  - If a handler blocks inside the OS before it exits, the Rockbox thread
 *    can run. Disabling interrupts then waits until the handler has exited,
 *    like the emulation, so the handler still runs alone.
 *
 * Rules this relies on: no code makes a blocking OS call with interrupts
 * masked, and handlers do not block with interrupts masked. */

#include <pspkernel.h>
#include <pspintrman.h>
#include "system-console.h"
#include "kernel.h"
#include "panic.h"
#include "sys_irq_psp.h"

/* Rockbox side (main OS thread) */
static volatile int rb_level = HIGHEST_IRQ_LEVEL;  /* until Rockbox enables */
static int rb_masked;              /* rb_saved holds a real SuspendIntr state */
static unsigned int rb_saved;
static volatile int rb_enabled_once;

/* handler side */
static SceLwMutexWorkarea handler_lock;
static volatile int handler_active;      /* between enter and exit */
static volatile SceUID handler_owner;
static int h_level;                      /* the active handler's own level */
static unsigned int h_saved;

static SceUID idle_sema = -1;
static volatile int inited;

static int in_handler(void)
{
    return handler_active && handler_owner == sceKernelGetThreadId();
}

int set_irq_level(int level)
{
    if (UNLIKELY(handler_active) && in_handler()) {
        int old = h_level;
        if (level && !old)
            h_saved = sceKernelCpuSuspendIntr();
        else if (!level && old)
            sceKernelCpuResumeIntr(h_saved);
        h_level = level;
        return old;
    }

    int old = rb_level;
    if (level && !old) {
        for (;;) {
            unsigned int s = sceKernelCpuSuspendIntr();
            if (!handler_active) {
                rb_saved = s;
                rb_masked = 1;
                break;
            }
            /* a handler blocked before exiting: let it finish */
            sceKernelCpuResumeIntr(s);
            sceKernelDelayThread(100);
        }
        rb_level = level;
    } else if (!level && old) {
        rb_level = 0;
        rb_enabled_once = 1;
        if (rb_masked) {
            rb_masked = 0;
            sceKernelCpuResumeIntr(rb_saved);
        }
    } else {
        rb_level = level;
    }
    return old;
}

void sim_enter_irq_handler(void)
{
    /* like hardware before the first enable_irq(): no handler runs */
    while (!rb_enabled_once)
        sceKernelDelayThread(1000);
    sceKernelLockLwMutex(&handler_lock, 1, NULL);
    handler_owner = sceKernelGetThreadId();
    h_level = 0;
    handler_active = 1;
}

void sim_exit_irq_handler(void)
{
    if (h_level) {                  /* unbalanced disable in a handler */
        h_level = 0;
        sceKernelCpuResumeIntr(h_saved);
    }
    handler_active = 0;
    handler_owner = 0;
    sceKernelUnlockLwMutex(&handler_lock, 1);
    sceKernelSignalSema(idle_sema, 1);
}

/* core_sleep(): idle until a handler has run, one tick at most */
void wait_for_interrupt(void)
{
    if (idle_sema >= 0) {
        SceUInt timeout = 1000000 / HZ;
        sceKernelWaitSema(idle_sema, 1, &timeout);
    }
}

void sys_irq_init(void)
{
    if (inited)
        return;
    if (sceKernelCreateLwMutex(&handler_lock, "rb_irq", 0, 0, NULL) < 0)
        panicf("Cannot create kernel objects");
    idle_sema = sceKernelCreateSema("rb_idle", 0, 0, 1, NULL);
    if (idle_sema < 0)
        panicf("Cannot create kernel objects");
    inited = 1;
}

void sys_irq_quit(void)
{
    enable_irq();
    while (handler_active)
        sceKernelDelayThread(10000);
}
