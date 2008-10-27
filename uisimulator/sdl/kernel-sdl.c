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
#include <SDL.h>
#include <SDL_thread.h>
#include "memory.h"
#include "system-sdl.h"
#include "uisdl.h"
#include "kernel.h"
#include "thread-sdl.h"
#include "thread.h"
#include "debug.h"

static SDL_TimerID tick_timer_id;
long start_tick;

/* Condition to signal that "interrupts" may proceed */
static SDL_cond *sim_thread_cond;
/* Mutex to serialize changing levels and exclude other threads while
 * inside a handler */
static SDL_mutex *sim_irq_mtx;
static int interrupt_level = HIGHEST_IRQ_LEVEL;
static int handlers_pending = 0;
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
        /* Not in a handler and "interrupts" are being reenabled */
        if (handlers_pending > 0)
            SDL_CondSignal(sim_thread_cond);
    }

    interrupt_level = level; /* save new level */

    SDL_UnlockMutex(sim_irq_mtx);
    return oldlevel;
}

void sim_enter_irq_handler(void)
{
    SDL_LockMutex(sim_irq_mtx);
    handlers_pending++;

    if(interrupt_level != 0)
    {
        /* "Interrupts" are disabled. Wait for reenable */
        SDL_CondWait(sim_thread_cond, sim_irq_mtx);
    }

    status_reg = 1;
}

void sim_exit_irq_handler(void)
{
    if (--handlers_pending > 0)
        SDL_CondSignal(sim_thread_cond);

    status_reg = 0;
    SDL_UnlockMutex(sim_irq_mtx);
}

bool sim_kernel_init(void)
{
    sim_irq_mtx = SDL_CreateMutex();
    if (sim_irq_mtx == NULL)
    {
        fprintf(stderr, "Cannot create sim_handler_mtx\n");
        return false;
    }

    sim_thread_cond = SDL_CreateCond();
    if (sim_thread_cond == NULL)
    {
        fprintf(stderr, "Cannot create sim_thread_cond\n");
        return false;
    }

    return true;
}

void sim_kernel_shutdown(void)
{
    SDL_RemoveTimer(tick_timer_id);
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
    
    return 1;
}

void tick_start(unsigned int interval_in_ms)
{
    if (tick_timer_id != NULL)
    {
        SDL_RemoveTimer(tick_timer_id);
        tick_timer_id = NULL;
    }
    else
    {
        start_tick = SDL_GetTicks();
    }

    tick_timer_id = SDL_AddTimer(interval_in_ms, tick_timer, NULL);
}
