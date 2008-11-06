/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "panic.h"

void INT_TIMER2(void)
{
    call_tick_tasks();  /* Run through the list of tick tasks */

    TIMER2_INTCLR = 0;  /* clear interrupt */
}

void tick_start(unsigned int interval_in_ms)
{
#ifdef BOOTLOADER
    (void) interval_in_ms;
#else
    int phi = 0;                            /* prescaler bits */
    int prescale = 1;
    int cycles = 64000 * interval_in_ms;    /* pclk is clocked at 64MHz */

    while(cycles > 0x10000)
    {
        phi++;
        prescale <<= 4;
        cycles >>= 4;
    }

    if(prescale > 256)
        panicf("%s : interval too big", __func__);

    CGU_PERI |= CGU_TIMER2_CLOCK_ENABLE;    /* enable peripheral */
    VIC_INT_ENABLE = INTERRUPT_TIMER2;      /* enable interrupt */

    TIMER2_LOAD = TIMER2_BGLOAD = cycles;   /* timer period */

    /* /!\ bit 4 (reserved) must not be modified
     * periodic mode, interrupt enabled, 16 bits counter */
    TIMER2_CONTROL = (TIMER2_CONTROL & (1<<4)) | 0xe0 | (phi<<2);
#endif
}
