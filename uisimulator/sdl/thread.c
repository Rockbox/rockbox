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

#include "autoconf.h"

#include <stdio.h>

/* SDL threading wrapper */
#include <SDL.h>
#include <SDL_thread.h>

#include "kernel.h"
#include <sys/time.h>

#ifdef ROCKBOX_HAS_SIMSOUND
#include "sound.h"
#endif

long current_tick = 0;
extern void sim_tick_tasks(void);

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
static int update_tick_thread(void* p)
{
    struct timeval start, now;
    long new_tick;

    (void)p;

    gettimeofday(&start, NULL);
    while (1)
    {
        msleep(5); /* check twice per simulated target tick */
        gettimeofday(&now, NULL);
        new_tick = (now.tv_sec - start.tv_sec) * HZ
                   + (now.tv_usec - start.tv_usec) / (1000000/HZ);
        if (new_tick > current_tick)
        {
            sim_tick_tasks();
            current_tick = new_tick;
        }
    }
}

/*
 * We emulate the target threads by using SDL threads. We have a mutex
 * that only allows one thread at a time to execute. It forces each
 * thread to yield() for the other(s) to run.
 */

SDL_mutex * mp;

void init_threads(void)
{  
    SDL_Thread *tick_tid;

    mp=SDL_CreateMutex();
    /* get mutex to only allow one thread running at a time */
    SDL_mutexP(mp);

    /* start a tick thread */
    tick_tid=SDL_CreateThread(update_tick_thread, NULL);

#ifdef ROCKBOX_HAS_SIMSOUND /* start thread that plays PCM data */
    {
        SDL_Thread *sound_tid;
        sound_tid = SDL_CreateThread(sound_playback_thread, NULL);
    }
#endif

}

void yield(void)
{
    SDL_mutexV(mp); /* return */
    msleep(1);                 /* prevent busy loop */
    SDL_mutexP(mp);   /* get it again */
}

void newfunc(void (*func)(void))
{
    SDL_mutexP(mp);
    func();
    SDL_mutexV(mp); 
}


int create_thread(void (*fp)(void), void* sp, int stk_size)
{
    SDL_Thread * tid;
    int i;
    int error;

    /* we really don't care about these arguments */
    (void)sp;
    (void)stk_size;
    tid = SDL_CreateThread(
                           (int(*)(void *))newfunc, /* function to start */
                           fp      /* start argument */);
    if(0 == tid) /* don't really have an error number here. */
        fprintf(stderr, "Couldn't run thread number %d\n", i);
    else
        fprintf(stderr, "Thread %d is running\n", (int)SDL_GetThreadID(tid));

    yield();

    return error;
}

void sim_sleep(int ticks)
{
    SDL_mutexV(mp); /* return */
    msleep((1000/HZ) * ticks);
    SDL_mutexP(mp);   /* get it again */
}

