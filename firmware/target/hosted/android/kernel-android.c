/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Thomas Martitz
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


#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include "config.h"
#include "system.h"
#include "button.h"
#include "audio.h"
#include "panic.h"


static sem_t wfi_sem;
/*
 * call tick tasks and wake the scheduler up */
void timer_signal(int sig)
{
    (void)sig;
    call_tick_tasks();
    interrupt();
}

/*
 * wait on the sem which the signal handler posts to save cpu time (aka sleep)
 *
 * other mechanisms could use them as well */
void wait_for_interrupt(void)
{
    sem_wait(&wfi_sem);
}

void interrupt(void)
{
    int s;
    /* unless idling, we usually have more interrupt() than wait_for_interrupt()
     * don't post unecessarily because wait_for_interrupt() would need to
     * decrement for each wasted sem_post(), instead of sleeping directly */
    sem_getvalue(&wfi_sem, &s);
    if (s <= 0)
        sem_post(&wfi_sem);
}

/*
 * setup a hrtimer to send a signal to our process every tick */
void tick_start(unsigned int interval_in_ms)
{
    int ret = 0;
    sigset_t proc_set;
    timer_t timerid;
    struct itimerspec ts;
    sigevent_t sigev = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo  = SIGUSR2,
    };

    ts.it_value.tv_sec = ts.it_interval.tv_sec = 0;
    ts.it_value.tv_nsec = ts.it_interval.tv_nsec = interval_in_ms*1000*1000;

    /* add the signal handler */
    signal(SIGUSR2, timer_signal);

    /* add the timer */
    ret |= timer_create(CLOCK_REALTIME, &sigev, &timerid);
    ret |= timer_settime(timerid, 0, &ts, NULL);

    /* unblock SIGUSR2 so the handler can run */
    ret |= sigprocmask(0, NULL, &proc_set);
    ret |= sigdelset(&proc_set, SIGUSR2);
    ret |= sigprocmask(SIG_SETMASK, &proc_set, NULL);

    ret |= sem_init(&wfi_sem, 0, 0);

    if (ret != 0)
        panicf("%s(): %s\n", __func__, strerror(errno));
}


bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void))
{
    (void)reg_prio;
    (void)unregister_callback;
    (void)cycles;
    (void)timer_callback;
    return false;
}

bool timer_set_period(long cycles)
{
    (void)cycles;
    return false;
}

void timer_unregister(void)
{
}
