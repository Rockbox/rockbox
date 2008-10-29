/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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

#ifndef BOOTLOADER
void TIMER1(void)
{
    /* Run through the list of tick tasks (using main core) */
    TIMER1_VAL; /* Read value to ack IRQ */

    /* Run through the list of tick tasks using main CPU core - 
       wake up the COP through its control interface to provide pulse */
    call_tick_tasks();

#if NUM_CORES > 1
    /* Pulse the COP */
    core_wake(COP);
#endif /* NUM_CORES */
}
#endif

/* Must be last function called init kernel/thread initialization */
void tick_start(unsigned int interval_in_ms)
{
#ifndef BOOTLOADER
    TIMER1_CFG = 0x0;
    TIMER1_VAL;
    /* enable timer */
    TIMER1_CFG = 0xc0000000 | (interval_in_ms*1000 - 1);
    /* unmask interrupt source */
    CPU_INT_EN = TIMER1_MASK;
#else
    /* We don't enable interrupts in the bootloader */
    (void)interval_in_ms;
#endif
}
