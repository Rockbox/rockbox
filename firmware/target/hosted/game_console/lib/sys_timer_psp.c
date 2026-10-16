/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Dan Everton
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sys_types.h"

#include "debug.h"
#include "logf.h"

static bool ticks_started = false;
static u64 _start_tick = 0;

static u64 system_tick(void)
{
    u64 ticks;
    sceRtcGetCurrentTick(&ticks);
    return ticks;
}

void sys_ticks_init(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = true;
    _start_tick = system_tick();

    /* DEBUGF("tick resolution: %ld\n", sceRtcGetTickResolution()); */
    /* DEBUGF("start_tick: %d\n", _start_tick); */
}

void sys_ticks_quit(void)
{
    ticks_started = false;
}

u64 sys_get_ticks64(void)
{
    u64 elapsed;
    if (!ticks_started) {
        sys_ticks_init();
    }

    elapsed = system_tick() - _start_tick;
    return elapsed / ((u64) sceRtcGetTickResolution() / 1000);
}

u32 sys_get_ticks(void)
{
    return (u32)(sys_get_ticks64() & 0xFFFFFFFF);
}

void sys_delay(u32 ms)
{
    const u32 max_delay = 0xffffffffUL / 1000;
    if (ms > max_delay) {
        ms = max_delay;
    }
    sceKernelDelayThreadCB(ms * 1000);
}