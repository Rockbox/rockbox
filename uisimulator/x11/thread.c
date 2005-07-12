/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <pthread.h>

#include "kernel.h"
#include <sys/time.h>

long current_tick = 0;
extern void button_tick(void);

static void msleep(int msec)
{
    struct timeval delay;
    
    delay.tv_sec = msec / 1000;
    delay.tv_usec = (msec % 1000) * 1000;
    select(0, NULL, NULL, NULL, &delay); /* portable sub-second sleep */
}

/*
 * This is not a target thread, so it does not fall under the 1 thread at a
 * time thing.
 */
static void update_tick_thread()
{
    struct timeval start, now;
    long new_tick;

    gettimeofday(&start, NULL);
    while (1)
    {
        msleep(5); /* check twice per simulated target tick */
        gettimeofday(&now, NULL);
        new_tick = (now.tv_sec - start.tv_sec) * HZ
                   + (now.tv_usec - start.tv_usec) / (1000000/HZ);
        if (new_tick > current_tick)
        {
            current_tick = new_tick;
            button_tick();  /* Dirty call to button.c. This should probably
                             * be implemented as a tick task the same way 
                             * as on the target. */
        }
    }
}

/*
 * We emulate the target threads by using pthreads. We have a mutex that only
 * allows one thread at a time to execute. It forces each thread to yield()
 * for the other(s) to run.
 */

pthread_mutex_t mp;

void init_threads(void)
{  
    pthread_t tick_tid;

    pthread_mutex_init(&mp, NULL);
    /* get mutex to only allow one thread running at a time */
    pthread_mutex_lock(&mp);

    pthread_create(&tick_tid, NULL, (void *(*)(void *)) update_tick_thread, 
                   NULL);
}
/* 
   int pthread_create(pthread_t *new_thread_ID,
   const pthread_attr_t *attr,
   void * (*start_func)(void *), void *arg);
*/

void yield(void)
{
    pthread_mutex_unlock(&mp); /* return */
    msleep(1);                 /* prevent busy loop */
    pthread_mutex_lock(&mp);   /* get it again */
}

void newfunc(void (*func)(void))
{
    pthread_mutex_lock(&mp);
    func();
    pthread_mutex_unlock(&mp); 
}


int create_thread(void (*fp)(void), void* sp, int stk_size)
{
    pthread_t tid;
    int i;
    int error;

    /* we really don't care about these arguments */
    (void)sp;
    (void)stk_size;
    error = pthread_create(&tid,
                           NULL,   /* default attributes please */
                           (void *(*)(void *)) newfunc, /* function to start */
                           fp      /* start argument */);
    if(0 != error)
        fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
    else
        fprintf(stderr, "Thread %ld is running\n", (long)tid);

    yield();

    return error;
}

void sim_sleep(int ticks)
{
    pthread_mutex_unlock(&mp); /* return */
    msleep((1000/HZ) * ticks);
    pthread_mutex_lock(&mp);   /* get it again */
}

