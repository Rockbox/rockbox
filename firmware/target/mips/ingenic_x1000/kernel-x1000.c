/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#include "kernel.h"
#include "system.h"
#include "x1000/ost.h"

/* TODO: implement a CPU frequency switching policy based on CPU utilization
 *
 * The basic assumption is that the workload consumes a fixed number of CPU
 * cycles per second on average (= utilization), so we can set the frequency
 * based on that value. Audio playback should fit this usage pattern well, so
 * it's a good fit for Rockbox.
 *
 * It's easier to understand in terms of fluid flow -- we need to keep
 * a reservoir of water topped up, CPU frequency is the inflow rate, and
 * CPU utilization is the outflow rate. The goal is to avoid running dry
 * and minimize the inflow rate.
 *
 * The only tricky part here is handing usage spikes -- CPU frequency has to
 * increase faster than utilization or there's a risk of audio dropouts.
 *
 * Rockbox CPU boost could be used as a hint to scale up frequency faster.
 * If that's not necessary to get good results, HAVE_ADJUSTABLE_CPU_FREQ can
 * be disabled entirely.
 */

#define CPU_IDLE_SAMPLES 100

void tick_start(unsigned interval_in_ms)
{
    jz_writef(OST_CTRL, PRESCALE1_V(BY_16));
    jz_write(OST_1DFR, interval_in_ms*(X1000_EXCLK_FREQ/16000));
    jz_write(OST_1CNT, 0);
    jz_write(OST_1FLG, 0);
    jz_write(OST_1MSK, 0);
    jz_setf(OST_ENABLE, OST1);
}

void OST(void)
{
    /* CPU idle time accounting */
    uint32_t now = __ost_read32();
    uint32_t div = now - __cpu_idle_reftick;
    if(div != 0) {
        uint32_t fraction = 1000 * __cpu_idle_ticks / div;
        __cpu_idle_avg += fraction - __cpu_idle_avg / CPU_IDLE_SAMPLES;
        __cpu_idle_cur = __cpu_idle_avg / CPU_IDLE_SAMPLES;
        __cpu_idle_ticks = 0;
        __cpu_idle_reftick = now;
    }

    /* Call regular kernel tick */
    jz_write(OST_1FLG, 0);
    call_tick_tasks();
}
