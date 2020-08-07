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

void SDL_TicksInit(void)
{
#ifdef USE_TIMER
    rb->timer_register(1, NULL, TIMER_FREQ/1000, sdltime IF_COP(, CPU));
#else
    start = *rb->current_tick;
#endif
}


void
SDL_TicksQuit(void)
{
#ifdef USE_TIMER
    rb->timer_unregister();
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

Uint64
SDL_GetPerformanceCounter(void)
{
    return SDL_GetTicks();
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 1000;
}

#endif
