/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "clock-stm32h7.h"
#include "mutex.h"
#include "panic.h"
#include <stdint.h>

struct stm_clock_state
{
    struct mutex mutex;
    uint8_t refcount[STM_NUM_CLOCKS];
};

static struct stm_clock_state stm_clocks;

void stm_clock_init(void)
{
    mutex_init(&stm_clocks.mutex);

    stm_target_clock_init();
}

void stm_clock_enable(enum stm_clock clock)
{
    mutex_lock(&stm_clocks.mutex);

    if (stm_clocks.refcount[clock] == UINT8_MAX)
        panicf("%s: clock %d overflow", __func__, (int)clock);

    if (stm_clocks.refcount[clock]++ == 0)
        stm_target_clock_enable(clock, true);

    mutex_unlock(&stm_clocks.mutex);
}

void stm_clock_disable(enum stm_clock clock)
{
    mutex_lock(&stm_clocks.mutex);

    if (stm_clocks.refcount[clock] == 0)
        panicf("%s: clock %d underflow", __func__, (int)clock);

    if (--stm_clocks.refcount[clock] == 0)
        stm_target_clock_enable(clock, false);

    mutex_unlock(&stm_clocks.mutex);
}
