/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include "SDL_config.h"

#if SDL_THREAD_ROCKBOX

/* Thread management routines for rockbox */

#include "SDL_thread.h"

#include "../SDL_systhread.h"
#include "../SDL_thread_c.h"

#include "plugin.h"

static struct semaphore sem;
static bool sem_init = false;
static void *global_args;

static void rbsdl_runthread(void)
{
    SDL_RunThread(global_args);
    rb->semaphore_release(&sem);
}

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
    /* memory leak!!! */
    rb->splash(HZ, "> CreateThread");
    const size_t sz = DEFAULT_STACK_SIZE;
    void *stack = malloc(sz);

    if(!sem_init)
    {
        rb->splash(HZ, "semaphore init");
        rb->semaphore_init(&sem, 1, 1);
        sem_init = true;
    }

    rb->semaphore_wait(&sem, TIMEOUT_BLOCK);

    char *name = malloc(16);
    static int threadnum = 0;
    rb->snprintf(name, 16, "sdl_%d", threadnum++);

    rb->splash(HZ, "create");

    global_args = args;

    thread->handle = rb->create_thread(rbsdl_runthread, stack, sz,
                                       0, name /* collisions allowed? */
                                       IF_PRIO(, PRIORITY_USER_INTERFACE)
                                       IF_COP(, CPU));

    rb->splash(HZ, "< CreateThread");
    return 0;
}

void SDL_SYS_SetupThread(void)
{
    return;
}

Uint32 SDL_ThreadID(void)
{
    return rb->thread_self();
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    rb->thread_wait(thread->handle);
}

void SDL_SYS_KillThread(SDL_Thread *thread)
{
#warning How do we do this?
    return;
}

#endif
