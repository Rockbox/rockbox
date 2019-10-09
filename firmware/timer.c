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

/* Register a user timer, called every <cycles> TIMER_FREQ cycles */
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void)
                    IF_COP(, int core))
{
    if (reg_prio <= timer_prio || cycles == 0)
        return false;

    if (!timer_set(cycles, true))
        return false;

    pfn_timer = timer_callback;
    /* NOTE: if unreg cb is defined you are in charge of calling timer_unregister() */
    pfn_unregister = unregister_callback;
    timer_prio = reg_prio;

    return timer_start(IF_COP(core));
}

bool timer_set_period(long cycles)
{
    return timer_set(cycles, false);
}

/* NOTE: unregister callbacks are not called by timer_unregister()
* the unregister_callback only gets called when your timer gets 
* overwritten by a lower priority timer using timer_register() */
void timer_unregister(void)
{
    timer_stop();

    pfn_timer = NULL;
    pfn_unregister = NULL;
    timer_prio = -1;
}

