/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/*
 * 3DS kernel support for the native Rockbox scheduler: emulated interrupts,
 * the kernel tick and the user timer (timer.h).
 *
 * All Rockbox threads run on the main libctru thread (kernel/thread.c).
 * Other libctru threads (this file's irq thread, pcm, recording, FS worker)
 * act as interrupt sources:
 *
 * - "Interrupts disabled" == the irq lock is held. The main thread takes it
 *   in set_irq_level() when the level goes from 0 to non-zero and releases it
 *   when the level returns to 0.
 * - An interrupt handler is code between sim_enter_irq_handler() and
 *   sim_exit_irq_handler(), which hold the same (recursive) lock, so handlers
 *   only run while Rockbox has interrupts enabled and never concurrently.
 * - Rockbox must never block at the OS level with interrupts disabled. Its
 *   idle loop (core_sleep) enables interrupts before wait_for_interrupt().
 *
 * The tick and user timer are 3DS kernel timers waited on by one libctru
 * thread, so nothing polls.
 */

#include <3ds.h>

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "timer.h"
#include "panic.h"
#include "debug.h"
#include "sys_timer.h"

#define IRQ_THREAD_STACK_SIZE  (32 * 1024)
#define IRQ_THREAD_PRIORITY    0x18  /* highest userland priority */

/* --- emulated interrupts ------------------------------------------------ */

static RecursiveLock irq_lock;
static int irq_level = HIGHEST_IRQ_LEVEL;
static LightEvent irq_event; /* signaled after every handler */

/* Runs on the main thread before main(): Rockbox starts with interrupts
 * disabled, so the main thread starts out holding the lock */
static void __attribute__((constructor)) irq_emul_init(void)
{
    RecursiveLock_Init(&irq_lock);
    RecursiveLock_Lock(&irq_lock);
    LightEvent_Init(&irq_event, RESET_ONESHOT);
}

int set_irq_level(int level)
{
    /* Serializes with handlers: also waits for a running handler */
    RecursiveLock_Lock(&irq_lock);

    int oldlevel = irq_level;
    irq_level = level;

    if (oldlevel == 0 && level != 0)
        return oldlevel;                    /* disabling: keep holding it */

    if (oldlevel != 0 && level == 0)
        RecursiveLock_Unlock(&irq_lock);    /* enabling: drop the hold */

    RecursiveLock_Unlock(&irq_lock);
    return oldlevel;
}

void sim_enter_irq_handler(void)
{
    RecursiveLock_Lock(&irq_lock);
}

void sim_exit_irq_handler(void)
{
    RecursiveLock_Unlock(&irq_lock);
    LightEvent_Signal(&irq_event);
}

/* core_sleep(): idle until a handler ran (or one tick passed) */
void wait_for_interrupt(void)
{
    LightEvent_WaitTimeout(&irq_event, 1000000000LL / HZ);
}

/* --- tick and user timer ------------------------------------------------ */

static u64 tick_start_ms; /* sys_get_ticks64() at tick_start() */

static Thread irq_thread;
static Handle quit_event;
static Handle tick_timer;
static Handle user_timer;

static int timer_prio = -1;
static void (*volatile user_timer_callback)(void);
static void (*unregister_cb)(void);

static void tick_handler(void)
{
    /* 64 bit: 32 bit milliseconds wrap after 49.7 days */
    long new_tick = (sys_get_ticks64() - tick_start_ms) / (1000/HZ);

    /* Catch up if this thread was late; each call advances current_tick */
    while (new_tick != current_tick)
    {
        sim_enter_irq_handler();
        call_tick_tasks();
        sim_exit_irq_handler();
    }
}

static void user_timer_handler(void)
{
    sim_enter_irq_handler();
    void (*callback)(void) = user_timer_callback;
    if (callback)
        callback();
    sim_exit_irq_handler();
}

static void irq_thread_run(void *arg)
{
    (void)arg;
    const Handle handles[3] = { quit_event, tick_timer, user_timer };

    while (1)
    {
        s32 index = -1;
        if (R_FAILED(svcWaitSynchronizationN(&index, handles, 3, false,
                                             U64_MAX)))
            continue;

        if (index == 0)
            break;
        else if (index == 1)
            tick_handler();
        else if (index == 2)
            user_timer_handler();
    }
}

void tick_start(unsigned int interval_in_ms)
{
    const s64 interval_ns = (s64)interval_in_ms * 1000000LL;

    if (irq_thread == NULL)
    {
        tick_start_ms = sys_get_ticks64();

        if (R_FAILED(svcCreateEvent(&quit_event, RESET_ONESHOT)) ||
            R_FAILED(svcCreateTimer(&tick_timer, RESET_ONESHOT)) ||
            R_FAILED(svcCreateTimer(&user_timer, RESET_ONESHOT)))
        {
            panicf("tick_start: cannot create kernel timers");
        }

        /* App core: core 1 (syscore) is used by the FS worker */
        irq_thread = threadCreate(irq_thread_run, NULL, IRQ_THREAD_STACK_SIZE,
                                  IRQ_THREAD_PRIORITY, -2, false);
        if (irq_thread == NULL)
            panicf("tick_start: cannot create irq thread");
    }

    svcSetTimer(tick_timer, interval_ns, interval_ns);
}

void sim_kernel_shutdown(void)
{
    if (irq_thread == NULL)
        return;

    /* the irq thread may be waiting to enter a handler */
    enable_irq();

    svcCancelTimer(user_timer);
    svcCancelTimer(tick_timer);
    svcSignalEvent(quit_event);
    threadJoin(irq_thread, U64_MAX);
    threadFree(irq_thread);
    irq_thread = NULL;

    svcCloseHandle(user_timer);
    svcCloseHandle(tick_timer);
    svcCloseHandle(quit_event);
}

/* timer.h: TIMER_FREQ cycles per second */
static void user_timer_set(long cycles)
{
    const s64 period_ns = (s64)cycles * 1000000000LL / TIMER_FREQ;
    svcSetTimer(user_timer, period_ns, period_ns);
}

bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void))
{
    if (reg_prio <= timer_prio || cycles <= 0)
        return false;

    /* like the native targets: tell the owner it lost the timer */
    if (unregister_cb != NULL) {
        void (*cb)(void) = unregister_cb;
        unregister_cb = NULL;
        cb();
    }

    int oldlevel = disable_irq_save();
    timer_prio = reg_prio;
    user_timer_callback = timer_callback;
    unregister_cb = unregister_callback;
    restore_irq(oldlevel);

    user_timer_set(cycles);
    return true;
}

bool timer_set_period(long cycles)
{
    if (cycles <= 0)
        return false;

    user_timer_set(cycles);
    return true;
}

void timer_unregister(void)
{
    svcCancelTimer(user_timer);
    svcClearTimer(user_timer); /* drop an expiry that is already pending */

    /* a handler holds the irq lock while calling the callback */
    int oldlevel = disable_irq_save();
    user_timer_callback = NULL;
    unregister_cb = NULL;
    timer_prio = -1;
    restore_irq(oldlevel);
}
