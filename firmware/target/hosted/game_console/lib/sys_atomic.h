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

#ifndef __SYSATOMIC_H__
#define __SYSATOMIC_H__

#include "sys_types.h"

#define AtomicAdd(ptr, value) __sync_fetch_and_add((u32 *)(ptr), value)

bool _AtomicCAS(u32 *ptr, int oldval, int newval);
#define AtomicGet(ptr) __atomic_load_n((u32*)(ptr), __ATOMIC_SEQ_CST)
#define AtomicSet(ptr, value) __sync_lock_test_and_set((u32 *)(ptr), value)
#define AtomicCAS(ptr, oldvalue, newvalue) _AtomicCAS((u32 *)(ptr), oldvalue, newvalue)
void AtomicLock(int *lock);
void AtomicUnlock(int *lock);

#endif /* #ifndef __SYSATOMIC_H__ */

