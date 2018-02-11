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

#ifdef SDL_TIMER_ROCKBOX

#include "SDL_timer.h"
#include "../SDL_timer_c.h"

#include "plugin.h"

/* color because greylib will use timer otherwise */
#if defined(HAVE_LCD_COLOR) && !defined(SIMULATOR) && !defined(RB_PROFILE)
#define USE_TIMER
#endif

/* based off doom */
#ifdef USE_TIMER
static volatile unsigned int ticks = 0;

static void sdltime(void)
{
    ticks++;
}
#else
static unsigned long start;
#endif

void SDL_StartTicks(void)
{
#ifdef USE_TIMER
    rb->timer_register(1, NULL, TIMER_FREQ/1000, sdltime IF_COP(, CPU));
#else
    start = *rb->current_tick;
#endif
}

Uint32 SDL_GetTicks (void)
{
#ifdef USE_TIMER
    return ticks;
#else
    return (*rb->current_tick - start) * (1000 / HZ);
#endif
}

void SDL_Delay (Uint32 ms)
{
    rb->sleep(ms / (1000 / HZ));
}

#include "SDL_thread.h"

/* Data to handle a single periodic alarm */
static int timer_alive = 0;
static SDL_Thread *timer = NULL;

static int RunTimer(void *unused)
{
        while ( timer_alive ) {
                if ( SDL_timer_running ) {
                        SDL_ThreadedTimerCheck();
                }
                SDL_Delay(1);
        }
        return(0);
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
        timer_alive = 1;
        timer = SDL_CreateThread(RunTimer, NULL);
        if ( timer == NULL )
                return(-1);
        return(SDL_SetTimerThreaded(1));
}

void SDL_SYS_TimerQuit(void)
{
        timer_alive = 0;
        if ( timer ) {
                SDL_WaitThread(timer, NULL);
                timer = NULL;
        }
}

int SDL_SYS_StartTimer(void)
{
        SDL_SetError("Internal logic error: threaded timer in use");
        return(-1);
}

void SDL_SYS_StopTimer(void)
{
        return;
}

#endif
