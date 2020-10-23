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


#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "button.h"
#include "audio.h"
#include "panic.h"
#include "timer.h"

/* Forward-declaration */
void interrupt(void);

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
 * setup a hrtimer to send a signal to our process every tick
 *
 * WARNING for Android: JNI calls are not permitted from tick tasks, as the
 * underlying thread is not attached to the Java VM
 *
 * Can be possibly be attached if it really needs to be. but let's
 * keep this leightweight
 */

void tick_start(unsigned int interval_in_ms)
{
    int ret = 0;
    timer_t timerid;
    struct itimerspec ts;
    sigevent_t sigev;

    /* initializing in the declaration causes some weird warnings */
    memset(&sigev, 0, sizeof(sigevent_t));
    sigev.sigev_notify = SIGEV_THREAD,
    sigev.sigev_notify_function = timer_signal;

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

#define cycles_to_microseconds(cycles) \
    ((int)((10000*cycles)/(TIMER_FREQ / 100)))


static timer_t timer_tid;
static int timer_prio = -1;
void (*global_unreg_callback)(void);
void (*global_timer_callback)(void);

static void timer_cb(union sigval arg)
{
    /* check for spurious callbacks [arg.sival_ptr] */
    if (global_timer_callback && global_timer_callback == arg.sival_ptr)
        global_timer_callback();
}

bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void))
{
    int ret = 0;
    struct itimerspec ts;
    sigevent_t sigev;
    long in_us = cycles_to_microseconds(cycles);

    if (reg_prio <= timer_prio || in_us <= 0)
        return false;

    if(timer_prio >= 0)
    {
        if (global_unreg_callback) /* timer has callback user needs to unreg */
            global_unreg_callback();
        else /* no callback -- delete timer */
            timer_delete(timer_tid);
    }

    memset(&sigev, 0, sizeof(sigevent_t));
    sigev.sigev_notify = SIGEV_THREAD,
    sigev.sigev_notify_function = timer_cb;
    sigev.sigev_value.sival_ptr = timer_callback; /* store cb to check later */

    div_t q = div(in_us, 1000000);
    ts.it_value.tv_sec = ts.it_interval.tv_sec = q.quot;
    ts.it_value.tv_nsec = ts.it_interval.tv_nsec = q.rem*1000;

    /* add the timer */
    ret |= timer_create(CLOCK_REALTIME, &sigev, &timer_tid);
    ret |= timer_settime(timer_tid, 0, &ts, NULL);

    global_timer_callback = timer_callback;
    global_unreg_callback = unregister_callback;
    timer_prio = reg_prio;

    return ret == 0;
}

bool timer_set_period(long cycles)
{
    struct itimerspec ts;
    long in_us = cycles_to_microseconds(cycles);
    div_t q = div(in_us, 1000000);
    ts.it_value.tv_sec = ts.it_interval.tv_sec = q.quot;
    ts.it_value.tv_nsec = ts.it_interval.tv_nsec = q.rem*1000;

    return timer_settime(timer_tid, 0, &ts, NULL) == 0;
}

void timer_unregister(void)
{
    timer_delete(timer_tid);
    timer_prio = -1;
    global_unreg_callback = NULL;
    global_timer_callback = NULL;
}
