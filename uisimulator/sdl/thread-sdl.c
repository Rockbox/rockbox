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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <time.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <stdlib.h>
#include "thread-sdl.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"

SDL_Thread          *threads[256];
int                 threadCount = 0;
long                current_tick = 0;
SDL_mutex *m;

void yield(void)
{
    static int counter = 0;

    SDL_mutexV(m);
    if (counter++ >= 50)
    {
        SDL_Delay(1);
        counter = 0;
    }
    SDL_mutexP(m);
}

void sim_sleep(int ticks)
{
    SDL_mutexV(m);
    SDL_Delay((1000/HZ) * ticks);
    SDL_mutexP(m);
}

int runthread(void *data)
{
    SDL_mutexP(m);
    ((void(*)())data) ();
    SDL_mutexV(m);
    return 0;
}

struct thread_entry* 
    create_thread(void (*function)(void), void* stack, int stack_size,
                  const char *name)
{
    /** Avoid compiler warnings */
    (void)stack;
    (void)stack_size;
    (void)name;
    SDL_Thread* t;

    if (threadCount == 256) {
        return NULL;
    }

    t = SDL_CreateThread(runthread, function);
    threads[threadCount++] = t;

    yield();

    /* The return value is never de-referenced outside thread.c so this
       nastiness should be fine.  However, a better solution would be nice.
      */
    return (struct thread_entry*)t;
}

void init_threads(void)
{
    m = SDL_CreateMutex();

    if (SDL_mutexP(m) == -1) {
        fprintf(stderr, "Couldn't lock mutex\n");
        exit(-1);
    }
}
