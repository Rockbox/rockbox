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

static void *global_args = NULL;

static void rbsdl_runthread(void)
{
    void *args = global_args;
    global_args = NULL;
    SDL_RunThread(args);
}

#define RBSDL_THREAD_STACK_SIZE (DEFAULT_STACK_SIZE * 4)
#define MAX_THREAD 4
static char names[MAX_THREAD][16];
static long stacks[MAX_THREAD][RBSDL_THREAD_STACK_SIZE / sizeof(long)];

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
    static int threadnum = 0;

    if(threadnum >= MAX_THREAD)
        return -1;

    snprintf(names[threadnum], 16, "sdl_%d", threadnum);

    while(global_args) rb->yield(); /* busy wait, pray that this works */

    global_args = args;

    thread->handle = rb->create_thread(rbsdl_runthread, stacks[threadnum], RBSDL_THREAD_STACK_SIZE,
                                       0, names[threadnum] /* collisions allowed? */
                                       IF_PRIO(, PRIORITY_BUFFERING) // this is used for sound mixing
                                       IF_COP(, CPU));

    threadnum++;

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
