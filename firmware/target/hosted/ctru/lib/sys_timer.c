/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "debug.h"
#include "logf.h"

#include <3ds/os.h>
#include "sys_thread.h"
#include "sys_timer.h"

#define CACHELINE_SIZE   128

static bool ticks_started = false;
static u64 start_tick;

#define NSEC_PER_MSEC 1000000ULL

void sys_ticks_init(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = true;

    start_tick = svcGetSystemTick();
}

void sys_ticks_quit(void)
{
    ticks_started = false;
}

u64 sys_get_ticks64(void)
{
    u64 elapsed;
    if (!ticks_started) {
        sys_ticks_init();
    }

    elapsed = svcGetSystemTick() - start_tick;
    return elapsed / CPU_TICKS_PER_MSEC;
}

u32 sys_get_ticks(void)
{
    return (u32)(sys_get_ticks64() & 0xFFFFFFFF);
}

void sys_delay(u32 ms)
{
    svcSleepThread(ms * NSEC_PER_MSEC);
}

typedef struct _timer
{
    int timerID;
    timer_callback_ptr callback;
    void *param;
    u32 interval;
    u32 scheduled;
    int canceled;
    struct _timer *next;
} sysTimer;

typedef struct _timer_map
{
    int timerID;
    sysTimer *timer;
    struct _timer_map *next;
} timerMap;

/* The timers are kept in a sorted list */
typedef struct
{
    /* Data used by the main thread */
    Thread thread;
    int nextID;
    timerMap *timermap;
    RecursiveLock timermap_lock;

    /* Padding to separate cache lines between threads */
    char cache_pad[CACHELINE_SIZE];

    /* Data used to communicate with the timer thread */
    int lock;
    LightSemaphore sem;
    sysTimer *pending;
    sysTimer *freelist;
    int active;

    /* List of timers - this is only touched by the timer thread */
    sysTimer *timers;
} timerData;

static timerData timer_data = { .active = 0 };

/* The idea here is that any thread might add a timer, but a single
 * thread manages the active timer queue, sorted by scheduling time.
 *
 * Timers are removed by simply setting a canceled flag
 */

static void add_timer_interval(timerData *data, sysTimer *timer)
{
    sysTimer *prev, *curr;

    prev = NULL;
    for (curr = data->timers; curr; prev = curr, curr = curr->next) {
        if ((s32)(timer->scheduled - curr->scheduled) < 0) {
            break;
        }
    }

    /* Insert the timer here! */
    if (prev) {
        prev->next = timer;
    } else {
        data->timers = timer;
    }
    timer->next = curr;
}

static void timer_thread(void *_data)
{
    timerData *data = (timerData *)_data;
    sysTimer *pending;
    sysTimer *current;
    sysTimer *freelist_head = NULL;
    sysTimer *freelist_tail = NULL;
    u32 tick, now, interval, delay;

    /* Threaded timer loop:
     *  1. Queue timers added by other threads
     *  2. Handle any timers that should dispatch this cycle
     *  3. Wait until next dispatch time or new timer arrives
     */
    for (;;) {
        /* Pending and freelist maintenance */
        AtomicLock(&data->lock);
        {
            /* Get any timers ready to be queued */
            pending = data->pending;
            data->pending = NULL;

            /* Make any unused timer structures available */
            if (freelist_head) {
                freelist_tail->next = data->freelist;
                data->freelist = freelist_head;
            }
        }
        AtomicUnlock(&data->lock);

        /* Sort the pending timers into our list */
        while (pending) {
            current = pending;
            pending = pending->next;
            add_timer_interval(data, current);
        }
        freelist_head = NULL;
        freelist_tail = NULL;

        /* Check to see if we're still running, after maintenance */
        if (!AtomicGet(&data->active)) {
            break;
        }

        /* Initial delay if there are no timers */
        delay = (~(u32)0);

        tick = sys_get_ticks();

        /* Process all the pending timers for this tick */
        while (data->timers) {
            current = data->timers;

            if ((s32)(tick - current->scheduled) < 0) {
                /* Scheduled for the future, wait a bit */
                delay = (current->scheduled - tick);
                break;
            }

            /* We're going to do something with this timer */
            data->timers = current->next;

            if (AtomicGet(&current->canceled)) {
                interval = 0;
            } else {
                interval = current->callback(current->interval, current->param);
            }

            if (interval > 0) {
                /* Reschedule this timer */
                current->interval = interval;
                current->scheduled = tick + interval;
                add_timer_interval(data, current);
            } else {
                if (!freelist_head) {
                    freelist_head = current;
                }
                if (freelist_tail) {
                    freelist_tail->next = current;
                }
                freelist_tail = current;

                AtomicSet(&current->canceled, 1);
            }
        }

        /* Adjust the delay based on processing time */
        now = sys_get_ticks();
        interval = (now - tick);
        if (interval > delay) {
            delay = 0;
        } else {
            delay -= interval;
        }

        /* Note that each time a timer is added, this will return
           immediately, but we process the timers added all at once.
           That's okay, it just means we run through the loop a few
           extra times.
         */
        sys_sem_wait_timeout(&data->sem, delay);
    }
}

int sys_timer_init(void)
{
    timerData *data = &timer_data;

    if (!AtomicGet(&data->active)) {
        RecursiveLock_Init(&data->timermap_lock);
        LightSemaphore_Init(&data->sem, 0, ((s16)0x7FFF));
        AtomicSet(&data->active, 1);

        /* Timer threads use a callback into the app, so we can't set a limited stack size here. */
        data->thread = threadCreate(timer_thread,
                                    data,
                                    32 * 1024,
                                    0x28,
                                    -1,
                                    false);
        if (!data->thread) {
            sys_timer_quit();
            return -1;
        }

        AtomicSet(&data->nextID, 1);
    }
    return 0;
}

void sys_timer_quit(void)
{
    timerData *data = &timer_data;
    sysTimer *timer;
    timerMap *entry;

    if (AtomicCAS(&data->active, 1, 0)) { /* active? Move to inactive. */
        /* Shutdown the timer thread */
        if (data->thread) {
            LightSemaphore_Release(&data->sem, 1);
            Result res = threadJoin(data->thread, U64_MAX);
            threadFree(data->thread);
            data->thread = NULL;
        }

        /* Clean up the timer entries */
        while (data->timers) {
            timer = data->timers;
            data->timers = timer->next;
            free(timer);
        }
        while (data->freelist) {
            timer = data->freelist;
            data->freelist = timer->next;
            free(timer);
        }
        while (data->timermap) {
            entry = data->timermap;
            data->timermap = entry->next;
            free(entry);
        }
    }
}

int sys_add_timer(u32 interval, timer_callback_ptr callback, void *param)
{
    timerData *data = &timer_data;
    sysTimer *timer;
    timerMap *entry;

    AtomicLock(&data->lock);
    if (!AtomicGet(&data->active)) {
        if (sys_timer_init() < 0) {
            AtomicUnlock(&data->lock);
            return 0;
        }
    }

    timer = data->freelist;
    if (timer) {
        data->freelist = timer->next;
    }
    AtomicUnlock(&data->lock);

    if (timer) {
        sys_remove_timer(timer->timerID);
    } else {
        timer = (sysTimer *) malloc(sizeof(*timer));
        if (!timer) {
            DEBUGF("sys_add_timer: out of memory\n");
            return 0;
        }
    }
    timer->timerID = AtomicIncrement(&data->nextID);
    timer->callback = callback;
    timer->param = param;
    timer->interval = interval;
    timer->scheduled = sys_get_ticks() + interval;
    AtomicSet(&timer->canceled, 0);

    entry = (timerMap *) malloc(sizeof(*entry));
    if (!entry) {
        free(timer);
        DEBUGF("sys_add_timer: out of memory\n");
        return 0;
    }
    entry->timer = timer;
    entry->timerID = timer->timerID;

    RecursiveLock_Lock(&data->timermap_lock);
    entry->next = data->timermap;
    data->timermap = entry;
    RecursiveLock_Unlock(&data->timermap_lock);

    /* Add the timer to the pending list for the timer thread */
    AtomicLock(&data->lock);
    timer->next = data->pending;
    data->pending = timer;
    AtomicUnlock(&data->lock);

    /* Wake up the timer thread if necessary */
    LightSemaphore_Release(&data->sem, 1);

    return entry->timerID;
}

bool sys_remove_timer(int id)
{
    timerData *data = &timer_data;
    timerMap *prev, *entry;
    bool canceled = false;

    /* Find the timer */
    RecursiveLock_Lock(&data->timermap_lock);
    prev = NULL;
    for (entry = data->timermap; entry; prev = entry, entry = entry->next) {
        if (entry->timerID == id) {
            if (prev) {
                prev->next = entry->next;
            } else {
                data->timermap = entry->next;
            }
            break;
        }
    }
    RecursiveLock_Unlock(&data->timermap_lock);

    if (entry) {
        if (!AtomicGet(&entry->timer->canceled)) {
            AtomicSet(&entry->timer->canceled, 1);
            canceled = true;
        }
        free(entry);
    }
    return canceled;
}

