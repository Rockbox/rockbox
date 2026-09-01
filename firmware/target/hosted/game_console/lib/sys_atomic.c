/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Mauricio G.
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
#include "debug.h"
#include "logf.h"

#include "sys_atomic.h"

bool _AtomicCAS(u32 *ptr, int oldval, int newval)
{
    return (bool) __sync_bool_compare_and_swap(ptr, oldval, newval);
}

bool _AtomicTryLock(int *lock)
{
    return __sync_lock_test_and_set(lock, 1) == 0;
}

void sys_delay(u32 ms);
#define CPUPauseInstruction()
void AtomicLock(int *lock)
{
    int iterations = 0;
    while (!_AtomicTryLock(lock)) {
        if (iterations < 32) {
            iterations++;
            CPUPauseInstruction();
        } else {
            sys_delay(0);
        }
    }
}

void AtomicUnlock(int *lock)
{
    __sync_lock_release(lock);
}
