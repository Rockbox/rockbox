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
#if defined CPU_PP
static long SHAREDBSS_ATTR cycles_new = 0;
#endif

#ifndef __TIMER_SET
/* Define these if not defined by target to make the #else cases compile
 * even if the target doesn't have them implemented. */
#define __TIMER_SET(cycles, set) false
#define __TIMER_START() false
#define __TIMER_STOP(...)
#endif

/* interrupt handler */
#if CONFIG_CPU == SH7034
void IMIA4(void) __attribute__((interrupt_handler));
void IMIA4(void)
{
    if (pfn_timer != NULL)
        pfn_timer();
    and_b(~0x01, &TSR4); /* clear the interrupt */
}
#elif defined(CPU_PP)
void TIMER2(void)
{
    TIMER2_VAL; /* ACK interrupt */
    if (cycles_new > 0)
    {
        TIMER2_CFG = 0xc0000000 | (cycles_new - 1);
        cycles_new = 0;
    }
    if (pfn_timer != NULL)
    {
        cycles_new = -1;
        /* "lock" the variable, in case timer_set_period()
         * is called within pfn_timer() */
        pfn_timer();
        cycles_new = 0;
    }
}
#endif /* CONFIG_CPU */

static bool timer_set(long cycles, bool start)
{
#if CONFIG_CPU == SH7034
    int phi = 0; /* bits for the prescaler */
    int prescale = 1;

    while (cycles > 0x10000)
    {   /* work out the smallest prescaler that makes it fit */
        phi++;
        prescale <<= 1;
        cycles >>= 1;
    }

    if (prescale > 8)
        return false;

    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }

        and_b(~0x10, &TSTR); /* Stop the timer 4 */
        and_b(~0x10, &TSNC); /* No synchronization */
        and_b(~0x10, &TMDR); /* Operate normally */

        TIER4 = 0xF9;        /* Enable GRA match interrupt */
    }

    TCR4 = 0x20 | phi;   /* clear at GRA match, set prescaler */
    GRA4 = (unsigned short)(cycles - 1);
    if (start || (TCNT4 >= GRA4))
        TCNT4 = 0;
    and_b(~0x01, &TSR4); /* clear an eventual interrupt */

    return true;
#elif defined(CPU_PP)
    if (cycles > 0x20000000 || cycles < 2)
        return false;

    if (start)
    {
        if (pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }
        CPU_INT_DIS = TIMER2_MASK;
        COP_INT_DIS = TIMER2_MASK;
    }
    if (start || (cycles_new == -1))  /* within isr, cycles_new is "locked" */
        TIMER2_CFG = 0xc0000000 | (cycles - 1);    /* enable timer */
    else
        cycles_new = cycles;

    return true;
#else
    return __TIMER_SET(cycles, start);
#endif /* CONFIG_CPU */
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

#if CONFIG_CPU == SH7034
    IPRD = (IPRD & 0xFF0F) | int_prio << 4;  /* interrupt priority */
    or_b(0x10, &TSTR); /* start timer 4 */
    return true;
#elif defined(CPU_PP)
    /* unmask interrupt source */
#if NUM_CORES > 1
    if (core == COP)
        COP_INT_EN = TIMER2_MASK;
    else
#endif
        CPU_INT_EN = TIMER2_MASK;
    return true;
#else
    return __TIMER_START();
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
#if CONFIG_CPU == SH7034
    and_b(~0x10, &TSTR);    /* stop the timer 4 */
    IPRD = (IPRD & 0xFF0F); /* disable interrupt */
#elif defined(CPU_PP)
    TIMER2_CFG = 0;         /* stop timer 2 */
    CPU_INT_DIS = TIMER2_MASK;
    COP_INT_DIS = TIMER2_MASK;
#else
    __TIMER_STOP();
#endif
    pfn_timer = NULL;
    pfn_unregister = NULL;
    timer_prio = -1;
}

