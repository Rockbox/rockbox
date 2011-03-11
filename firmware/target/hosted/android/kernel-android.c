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
#include <pthread.h>
#include "config.h"
#include "system.h"
#include "button.h"
#include "audio.h"
#include "panic.h"


static pthread_cond_t wfi_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t wfi_mtx = PTHREAD_MUTEX_INITIALIZER;
/*
 * call tick tasks and wake the scheduler up */
void timer_signal(union sigval arg)
{
    (void)arg;
    call_tick_tasks();
    interrupt();
}

/*
 * wait on the sem which the signal handler posts to save cpu time (aka sleep)
 *
 * other mechanisms could use them as well */
void wait_for_interrupt(void)
{
    pthread_cond_wait(&wfi_cond, &wfi_mtx);
}

/*
 * Wakeup the kernel, if sleeping (shall not be called from a signal handler) */
void interrupt(void)
{
    pthread_cond_signal(&wfi_cond);
}

/*
 * setup a hrtimer to send a signal to our process every tick */
void tick_start(unsigned int interval_in_ms)
{
    int ret = 0;
    timer_t timerid;
    struct itimerspec ts;
    sigevent_t sigev;

    /* initializing in the declaration causes some weird warnings */
    memset(&sigev, 0, sizeof(sigevent_t));
    sigev.sigev_notify = SIGEV_THREAD,
    sigev.sigev_notify_function = timer_signal,

    ts.it_value.tv_sec = ts.it_interval.tv_sec = 0;
    ts.it_value.tv_nsec = ts.it_interval.tv_nsec = interval_in_ms*1000*1000;

    /* add the timer */
    ret |= timer_create(CLOCK_REALTIME, &sigev, &timerid);
    ret |= timer_settime(timerid, 0, &ts, NULL);

    /* Grab the mutex already now and leave it to this thread. We don't
     * care about race conditions when signaling the condition (because
     * they are not critical), but a mutex is necessary due to the API */
    pthread_mutex_lock(&wfi_mtx);

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
