/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Jens Arnold
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

#include <stdbool.h>
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "timer.h"
#include "logf.h"

static int timer_prio = -1;
void SHAREDBSS_ATTR (*pfn_timer)(void) = NULL;      /* timer callback */
void SHAREDBSS_ATTR (*pfn_unregister)(void) = NULL; /* unregister callback */

#ifndef __TIMER_SET
/* Define these if not defined by target to make the #else cases compile
 * even if the target doesn't have them implemented. */
#define __TIMER_SET(cycles, set) false
#if NUM_CORES > 1
#define __TIMER_START(int_prio, core) false
#else
#define __TIMER_START(int_prio) false
#endif
#define __TIMER_STOP()
#endif

static bool timer_set(long cycles, bool start)
{
    return __TIMER_SET(cycles, start);
}

/* Register a user timer, called every <cycles> TIMER_FREQ cycles */
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, int int_prio, void (*timer_callback)(void)
                    IF_COP(, int core))
{
    if (reg_prio <= timer_prio || cycles == 0)
        return false;

#if CONFIG_CPU == SH7034
    if (int_prio < 1 || int_prio > 15)
        return false;
#endif

    if (!timer_set(cycles, true))
        return false;

    pfn_timer = timer_callback;
    pfn_unregister = unregister_callback;
    timer_prio = reg_prio;

#if NUM_CORES > 1
    return __TIMER_START(int_prio, core);
#else
    return __TIMER_START(int_prio);
#endif

    /* Cover for targets that don't use all these */
    (void)reg_prio;
    (void)unregister_callback;
    (void)cycles;
    /* TODO: Implement for PortalPlayer and iFP (if possible) */
    (void)int_prio;
    (void)timer_callback;
}

bool timer_set_period(long cycles)
{
    return timer_set(cycles, false);
}

void timer_unregister(void)
{
    __TIMER_STOP();

    pfn_timer = NULL;
    pfn_unregister = NULL;
    timer_prio = -1;
}

