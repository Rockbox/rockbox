/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Marcin Bukat
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

/*  rockchip rk27xx driver for the kernel timer */

/* sys timer ISR */
void INT_TIMER0(void)
{
    /* clear interrupt */
    TMR0CON &= ~0x04;

    call_tick_tasks();  /* Run through the list of tick tasks */
}

/* this assumes 50MHz APB bus frequency */
void tick_start(unsigned int interval_in_ms)
{
    unsigned int cycles = 50000 * interval_in_ms;
    
    /* enable timer clock */
    SCU_CLKCFG &= (1<<28);
    
    /* configure timer0 */
    TMR0LR = cycles;
    TMR0CON = (1<<8) | (1<<7) | (1<<1); /* periodic, 1/1, interrupt enable */

    /* unmask timer0 interrupt */
    INTC_IMR |= 0x04;

    /* enable timer0 interrupt */
    INTC_IECR |= 0x04;
}

