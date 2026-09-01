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

#include <3ds/os.h>

#include "debug.h"
#include "logf.h"

static bool ticks_started = false;
static u64 start_tick;

#define NSEC_PER_MSEC 1000000ULL

void sys_ticks_init(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = true;

    start_tick = svcGetSystemTick();
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

    elapsed = svcGetSystemTick() - start_tick;
    return elapsed / CPU_TICKS_PER_MSEC;
}

u32 sys_get_ticks(void)
{
    return (u32)(sys_get_ticks64() & 0xFFFFFFFF);
}

void sys_delay(u32 ms)
{
    svcSleepThread(ms * NSEC_PER_MSEC);
}