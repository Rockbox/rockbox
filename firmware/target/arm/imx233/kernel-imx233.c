/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "kernel.h"
#include "timrot-imx233.h"
#include "clkctrl-imx233.h"
#include "kernel-imx233.h"

static void tick_timer(void)
{
    /* Run through the list of tick tasks */
    call_tick_tasks();
}

void tick_start(unsigned int interval_in_ms)
{
    /* use the 1-kHz XTAL clock source */
    imx233_setup_timer(TICK_TIMER_NR, true, interval_in_ms,
        HW_TIMROT_TIMCTRL__SELECT_1KHZ_XTAL, HW_TIMROT_TIMCTRL__PRESCALE_1,
        false, &tick_timer);
}


void arbiter_init(struct channel_arbiter_t *a, unsigned count)
{
    mutex_init(&a->mutex);
    semaphore_init(&a->sema, count, count);
    a->free_bm = (1 << count) - 1;
    a->count = count;
}

// doesn't check in use !
void arbiter_reserve(struct channel_arbiter_t *a, unsigned channel)
{
    // assume semaphore has a free slot immediately
    if(semaphore_wait(&a->sema, TIMEOUT_NOBLOCK) != OBJ_WAIT_SUCCEEDED)
        panicf("arbiter_reserve failed on semaphore_wait !");
    mutex_lock(&a->mutex);
    a->free_bm &= ~(1 << channel);
    mutex_unlock(&a->mutex);
}

int arbiter_acquire(struct channel_arbiter_t *a, int timeout)
{
    int w = semaphore_wait(&a->sema, timeout);
    if(w == OBJ_WAIT_TIMEDOUT)
        return w;
    mutex_lock(&a->mutex);
    int chan = find_first_set_bit(a->free_bm);
    if(chan >= a->count)
        panicf("arbiter_acquire cannot find a free channel !");
    a->free_bm &= ~(1 << chan);
    mutex_unlock(&a->mutex);
    return chan;
}

bool arbiter_acquired(struct channel_arbiter_t *a, int channel)
{
    return !(a->free_bm & (1 << channel));
}

void arbiter_release(struct channel_arbiter_t *a, int channel)
{
    mutex_lock(&a->mutex);
    a->free_bm |= 1 << channel;
    mutex_unlock(&a->mutex);
    semaphore_release(&a->sema);
}
