/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
#include "cpu.h"

void tick_start(unsigned int interval_in_ms)
{
    unsigned int latch;

    /* 12Mhz / 4 = 3Mhz */
    latch = interval_in_ms*1000 * 3;

    REG_OST_OSTCSR = OSTCSR_PRESCALE4 | OSTCSR_EXT_EN;
    REG_OST_OSTDR = latch;
    REG_OST_OSTCNTL = 0;
    REG_OST_OSTCNTH = 0;

    system_enable_irq(IRQ_TCU0);

    REG_TCU_TMCR = TMCR_OSTMASK; /* unmask match irq */
    REG_TCU_TSCR = TSCR_OST;  /* enable timer clock */
    REG_TCU_TESR = TESR_OST;  /* start counting up */
}

/* Interrupt handler */
void TCU0(void)
{
    REG_TCU_TFCR = TFCR_OSTFLAG; /* ACK timer */
    
    /* Run through the list of tick tasks */
    call_tick_tasks();
}
