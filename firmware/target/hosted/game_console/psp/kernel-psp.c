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

/* Kernel support for the native scheduler on the PSP.
 * Replaces kernel-console.c and timer-console.c.
 *
 * Every Rockbox thread runs inside the main PSP OS thread. Other OS threads
 * (the tick and user timers here, the PCM thread) enter Rockbox like
 * interrupt handlers through sim_enter_irq_handler()/sim_exit_irq_handler().
 *
 * The interrupt support (set_irq_level, handler entry and exit, the idle
 * wait) is in lib/sys_irq_psp.c. */

#include <stdlib.h>
#include <pspkernel.h>
#include "system-console.h"
#include "kernel.h"
#include "thread.h"
#include "timer.h"
#include "panic.h"
#include "debug.h"

#include "sys_thread.h"
#include "sys_timer.h"
#include "sys_irq_psp.h"

/* ---- tick ---- */
static int tick_timer_id;
long start_tick;

static u32 tick_timer(u32 interval, void *param)
{
    (void)param;

    long new_tick = (sys_get_ticks() - start_tick) / (1000/HZ);

    while (new_tick != current_tick)
    {
        sim_enter_irq_handler();
        call_tick_tasks();          /* increments current_tick */
        sim_exit_irq_handler();
    }

    return interval;
}

void tick_start(unsigned int interval_in_ms)
{
    sys_irq_init();

    if (tick_timer_id != 0) {
        sys_remove_timer(tick_timer_id);
        tick_timer_id = 0;
    } else {
        start_tick = sys_get_ticks();
    }

    tick_timer_id = sys_add_timer(interval_in_ms, tick_timer, NULL);
}

void sim_kernel_shutdown(void)
{
    sys_remove_timer(tick_timer_id);
    sys_irq_quit();
}

/* ---- user timer (timer.h), also entered as an interrupt ---- */
static int timer_prio = -1;
static void (*user_timer_callback)(void);
static int user_timer_id;

static u32 user_timer(u32 interval, void *param)
{
    (void)param;
    sim_enter_irq_handler();
    user_timer_callback();
    sim_exit_irq_handler();
    return interval;
}

#define cycles_to_milliseconds(cycles) \
    ((int)((1000*(cycles))/TIMER_FREQ))

bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void))
{
    (void)unregister_callback;
    if (reg_prio <= timer_prio || cycles == 0)
        return false;
    timer_prio = reg_prio;
    user_timer_callback = timer_callback;
    user_timer_id = sys_add_timer(cycles_to_milliseconds(cycles), user_timer, NULL);
    return true;
}

bool timer_set_period(long cycles)
{
    sys_remove_timer(user_timer_id);
    user_timer_id = sys_add_timer(cycles_to_milliseconds(cycles), user_timer, NULL);
    return true;
}

void timer_unregister(void)
{
    sys_remove_timer(user_timer_id);
    timer_prio = -1;
}
