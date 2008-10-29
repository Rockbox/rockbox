/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Tomasz Malesinski
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
#include "config.h"
#include "system.h"
#include "kernel.h"

void timer_handler(void)
{
    /* Run through the list of tick tasks */
    call_tick_tasks();

    TIMER0.clr = 0;
}

void tick_start(unsigned int interval_in_ms)
{
    TIMER0.ctrl &= ~0x80; /* Disable the counter */
    TIMER0.ctrl |= 0x40;  /* Reload after counting down to zero */
    TIMER0.load = 3000000 * interval_in_ms / 1000;
    TIMER0.ctrl &= ~0xc;  /* No prescaler */
    TIMER0.clr = 1;       /* Clear the interrupt request */

    irq_set_int_handler(IRQ_TIMER0, timer_handler);
    irq_enable_int(IRQ_TIMER0);

    TIMER0.ctrl |= 0x80;  /* Enable the counter */
}
