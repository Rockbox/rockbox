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

/*
 * This is not a target thread, so it does not fall under the 1 thread at a
 * time thing.
 */
static void update_tick_thread()
{
    struct timeval start, now, delay;

    gettimeofday(&start, NULL);
    while (1)
    {
        delay.tv_sec = 0;
        delay.tv_usec = (1000000/HZ/4);  /* check 4 times per target tick */
        select(0, NULL, NULL, NULL, &delay); /* portable sub-second sleep */
        gettimeofday(&now, NULL);
        current_tick = (now.tv_sec - start.tv_sec) * HZ
                     + (now.tv_usec - start.tv_usec) * HZ / 1000000;
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
    pthread_mutex_lock(&mp); /* get it again */
}

void newfunc(void (*func)(void))
{
    pthread_mutex_lock(&mp);
    func();
    pthread_mutex_unlock(&mp); 
}


int create_thread(void* fp, void* sp, int stk_size)
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
    struct timeval delay;

    pthread_mutex_unlock(&mp); /* return */
    delay.tv_sec = ticks / HZ;
    delay.tv_usec = (ticks - HZ * delay.tv_sec) * (1000000/HZ);
    select(0, NULL, NULL, NULL, &delay);   /* portable subsecond sleep */
    pthread_mutex_lock(&mp); /* get it again */
}

void mutex_init(struct mutex *m)
{
    (void)m;
}

void mutex_lock(struct mutex *m)
{
    (void)m;
}

void mutex_unlock(struct mutex *m)
{
    (void)m;
}
