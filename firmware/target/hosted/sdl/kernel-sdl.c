/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
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
#include <SDL.h>
#include <SDL_thread.h>
#include <inttypes.h>
#include "system-sdl.h"
#include "thread-sdl.h"
#include "kernel.h"
#include "thread.h"
#include "panic.h"
#include "debug.h"

static SDL_TimerID tick_timer_id;
long start_tick;

#ifndef HAVE_SDL_THREADS
/* for the wait_for_interrupt function */
static SDL_cond *wfi_cond;
static SDL_mutex *wfi_mutex;
#endif
/* Condition to signal that "interrupts" may proceed */
static SDL_cond *sim_thread_cond;
/* Mutex to serialize changing levels and exclude other threads while
 * inside a handler */
static SDL_mutex *sim_irq_mtx;
/* Level: 0 = enabled, not 0 = disabled */
static int volatile interrupt_level = HIGHEST_IRQ_LEVEL;
/* How many handers waiting? Not strictly needed because CondSignal is a
 * noop if no threads were waiting but it filters-out calls to functions
 * with higher overhead and provides info when debugging. */
static int handlers_pending = 0;
/* 1 = executing a handler; prevents CondSignal calls in set_irq_level
 * while in a handler */
static int status_reg = 0;

/* Nescessary logic:
 * 1) All threads must pass unblocked
 * 2) Current handler must always pass unblocked
 * 3) Threads must be excluded when irq routine is running
 * 4) No more than one handler routine should execute at a time
 */
int set_irq_level(int level)
{
    SDL_LockMutex(sim_irq_mtx);

    int oldlevel = interrupt_level;

    if (status_reg == 0 && level == 0 && oldlevel != 0)
    {
        /* Not in a handler and "interrupts" are going from disabled to
         * enabled; signal any pending handlers still waiting */
        if (handlers_pending > 0)
            SDL_CondBroadcast(sim_thread_cond);
    }

    interrupt_level = level; /* save new level */

    SDL_UnlockMutex(sim_irq_mtx);
    return oldlevel;
}

void sim_enter_irq_handler(void)
{
    SDL_LockMutex(sim_irq_mtx);
    handlers_pending++;

    /* Check each time before proceeding: disabled->enabled->...->disabled
     * is possible on an app thread before a handler thread is ever granted
     * the mutex; a handler can also leave "interrupts" disabled during
     * its execution */
    while (interrupt_level != 0)
        SDL_CondWait(sim_thread_cond, sim_irq_mtx);

    status_reg = 1;
}

void sim_exit_irq_handler(void)
{
    /* If any others are waiting, give the signal */
    if (--handlers_pending > 0)
        SDL_CondSignal(sim_thread_cond);

    status_reg = 0;
    SDL_UnlockMutex(sim_irq_mtx);
#ifndef HAVE_SDL_THREADS
    SDL_CondSignal(wfi_cond);
#endif
}

static bool sim_kernel_init(void)
{
    sim_irq_mtx = SDL_CreateMutex();
    if (sim_irq_mtx == NULL)
    {
        panicf("Cannot create sim_handler_mtx\n");
        return false;
    }

    sim_thread_cond = SDL_CreateCond();
    if (sim_thread_cond == NULL)
    {
        panicf("Cannot create sim_thread_cond\n");
        return false;
    }
#ifndef HAVE_SDL_THREADS
    wfi_cond = SDL_CreateCond();
    if (wfi_cond == NULL)
    {
        panicf("Cannot create wfi\n");
        return false;
    }
    wfi_mutex = SDL_CreateMutex();
    if (wfi_mutex == NULL)
    {
        panicf("Cannot create wfi mutex\n");
        return false;
    }
#endif
    return true;
}

void sim_kernel_shutdown(void)
{
    SDL_RemoveTimer(tick_timer_id);
#ifndef HAVE_SDL_THREADS
    SDL_DestroyCond(wfi_cond);
    SDL_UnlockMutex(wfi_mutex);
    SDL_DestroyMutex(wfi_mutex);
#endif
    enable_irq();
    while(handlers_pending > 0)
        SDL_Delay(10);

    SDL_DestroyMutex(sim_irq_mtx);
    SDL_DestroyCond(sim_thread_cond);
}

Uint32 tick_timer(Uint32 interval, void *param)
{
    long new_tick;

    (void) interval;
    (void) param;

    new_tick = (SDL_GetTicks() - start_tick) / (1000/HZ);

    while(new_tick != current_tick)
    {
        sim_enter_irq_handler();

        /* Run through the list of tick tasks - increments tick
         * on each iteration. */
        call_tick_tasks();

        sim_exit_irq_handler();
    }

    return interval;
}

void tick_start(unsigned int interval_in_ms)
{
    if (!sim_kernel_init())
    {
        panicf("Could not initialize kernel!");
        exit(-1);
    }

    if (tick_timer_id != 0)
    {
        SDL_RemoveTimer(tick_timer_id);
        tick_timer_id = 0;
    }
    else
    {
        start_tick = SDL_GetTicks();
    }

    tick_timer_id = SDL_AddTimer(interval_in_ms, tick_timer, NULL);
#ifndef HAVE_SDL_THREADS
    SDL_LockMutex(wfi_mutex);
#endif
}

#ifndef HAVE_SDL_THREADS
void wait_for_interrupt(void)
{
    /* the exit may come at any time, during the CondWait or before,
     * so check it twice */
    SDL_CondWait(wfi_cond, wfi_mutex);
}
#endif
