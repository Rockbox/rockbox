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
#include <poll.h>

long current_tick = 0;

/*
 * We emulate the target threads by using pthreads. We have a mutex that only
 * allows one thread at a time to execute. It forces each thread to yield()
 * for the other(s) to run.
 */

pthread_mutex_t mp;

void init_threads(void)
{  
  pthread_mutex_init(&mp, NULL);
  /* get mutex to only allow one thread running at a time */
  pthread_mutex_lock(&mp);

  current_tick = time(NULL); /* give it a boost from start! */
}
/* 
   int pthread_create(pthread_t *new_thread_ID,
   const pthread_attr_t *attr,
   void * (*start_func)(void *), void *arg);
*/

void yield(void)
{
  current_tick+=3;
  pthread_mutex_unlock(&mp); /* return */
  pthread_mutex_lock(&mp); /* get it again */
}

void newfunc(void (*func)(void))
{
    yield();
    func();
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
                         (void *(*)(void *)) newfunc,   /* function to start */
                         fp      /* start argument */);
  if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
  else 
      fprintf(stderr, "Thread %ld is running\n", (long)tid);

  yield();

  return error;
}

/* ticks is HZ per second */
void sim_sleep(int ticks)
{
    current_tick+=5;
    pthread_mutex_unlock(&mp); /* return */
    /* portable subsecond "sleep" */
    poll((void *)0, 0, ticks * 1000/HZ);

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
