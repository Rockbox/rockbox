/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: timer.h 13806 2007-07-06 21:36:32Z jethead71 $
*
* Copyright (C) 2005 Kévin Ferrare
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

#include "timer.h"
#include <SDL_timer.h>

static int timer_prio = -1;
void (*global_timer_callback)(void);
SDL_TimerID timerId;

Uint32 SDL_timer_callback(Uint32 interval, void *param){
    (void)param;
    global_timer_callback();
    return(interval);
}

#define cycles_to_miliseconds(cycles) \
    ((int)((1000*cycles)/TIMER_FREQ))

bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, int int_prio, void (*timer_callback)(void))
{
    (void)int_prio;/* interrupt priority not used */
    (void)unregister_callback;
    if (reg_prio <= timer_prio || cycles == 0)
        return false;
    timer_prio=reg_prio;
    global_timer_callback=timer_callback;
    timerId=SDL_AddTimer(cycles_to_miliseconds(cycles), SDL_timer_callback, 0);
    return true;
}

bool timer_set_period(long cycles)
{
    SDL_RemoveTimer (timerId);
    timerId=SDL_AddTimer(cycles_to_miliseconds(cycles), SDL_timer_callback, 0);
    return true;
}

void timer_unregister(void)
{
    SDL_RemoveTimer (timerId);
    timer_prio = -1;
}
