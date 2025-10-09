/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2025 Mauricio G.
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

#include "sys_timer.h"
#include "timer.h"

static int timer_prio = -1;
void (*global_timer_callback)(void);
int timerId;

u32 _timer_callback(u32 interval, void *param){
    (void)param;
    global_timer_callback();
    return(interval);
}

#define cycles_to_miliseconds(cycles) \
    ((int)((1000*cycles)/TIMER_FREQ))

bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void))
{
    (void)unregister_callback;
    if (reg_prio <= timer_prio || cycles == 0)
        return false;
    timer_prio=reg_prio;
    global_timer_callback=timer_callback;
    timerId=sys_add_timer(cycles_to_miliseconds(cycles), _timer_callback, 0);
    return true;
}

bool timer_set_period(long cycles)
{
    sys_remove_timer(timerId);
    timerId=sys_add_timer(cycles_to_miliseconds(cycles), _timer_callback, 0);
    return true;
}

void timer_unregister(void)
{
    sys_remove_timer(timerId);
    timer_prio = -1;
}
