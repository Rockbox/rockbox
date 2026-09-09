/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/timer-x1000.c,
 * Copyright (C) 2021 Aidan MacDonald
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
#include "system.h"
#include "irq-x1600.h"
#include "x1600/tcu.h"

/* Use TCU channel 5. Keep its target-specific routing and setup here. */
#define TIMER_CHN 5

/* Channel 5 uses the X1000 TCU1 routing. This assignment is unverified on
 * X1600. */
#define TIMER_IRQ  IRQ_TCU1

bool timer_set(long cycles, bool start)
{
    if(cycles <= 0)
        return false;

    /* Calculate timer interval; the counter and compares are 16 bit */
    unsigned long counter = cycles;
    unsigned prescale = 0;
    while(counter > 0xffff && prescale < 5) {
        counter /= 4;
        prescale += 1;
    }

    /* Duration too long */
    if(counter > 0xffff)
        return false;

    /* Unregister old function */
    if(start && pfn_unregister) {
        pfn_unregister();
        pfn_unregister = 0;
    }

    bool enabled = jz_read(TCU_ENABLE) & (1 << TIMER_CHN);

    /* Configure the timer; TIMER_FREQ is EXCLK, which denominates cycles */
    jz_clr(TCU_STOP, 1 << TIMER_CHN);
    jz_clr(TCU_ENABLE, 1 << TIMER_CHN);
    /* EXT_EN(1) rather than the X1000's SOURCE_V(EXT): bit 2 either way */
    jz_overwritef(TCU_CTRL(TIMER_CHN), EXT_EN(1), PRESCALE(prescale));
    jz_write(TCU_CMP_FULL(TIMER_CHN), counter);
    jz_write(TCU_CMP_HALF(TIMER_CHN), 0);
    jz_clr(TCU_FLAG, 1 << TIMER_CHN);
    jz_clr(TCU_MASK, 1 << TIMER_CHN);

    /* Enable the controller IRQ when the timer is enabled. */
    system_enable_irq(TIMER_IRQ);

    /* timer_register() starts new timers after installing the callback. */
    if(!start && enabled)
        return timer_start();
    else
        return true;
}

bool timer_start(void)
{
    jz_set(TCU_ENABLE, 1 << TIMER_CHN);
    return true;
}

void timer_stop(void)
{
    jz_clr(TCU_ENABLE, 1 << TIMER_CHN);
    jz_set(TCU_MASK, 1 << TIMER_CHN);
    jz_clr(TCU_FLAG, 1 << TIMER_CHN);
    jz_set(TCU_STOP, 1 << TIMER_CHN);
    system_disable_irq(TIMER_IRQ);
}

/* The name must match system-x1600.c's irqvector[] entry for TIMER_IRQ */
void TCU1(void)
{
    jz_clr(TCU_FLAG, 1 << TIMER_CHN);

    if(pfn_timer)
        pfn_timer();
}
