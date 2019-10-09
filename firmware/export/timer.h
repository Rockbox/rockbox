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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdbool.h>
#include "config.h"
#include "cpu.h"

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
#define TIMER_FREQ 1000000
#endif

/* NOTE: if unreg cb is defined you are in charge of calling timer_unregister() */
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void)
                    IF_COP(,int core));
bool timer_set_period(long cycles);
#ifdef CPU_COLDFIRE
void timers_adjust_prescale(int multiplier, bool enable_irq);
#endif

/* NOTE: unregister callbacks are not called by timer_unregister()
* the unregister_callback only gets called when your timer gets 
* overwritten by a lower priority timer using timer_register() */
void timer_unregister(void);

/* target-specific interface */
bool timer_set(long cycles, bool start);
bool timer_start(IF_COP_VOID(int core));
void timer_stop(void);

/* For target-specific interface use */
extern void (*pfn_timer)(void);
extern void (*pfn_unregister)(void);

#endif /* __TIMER_H__ */
