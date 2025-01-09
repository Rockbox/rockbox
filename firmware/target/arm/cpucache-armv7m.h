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
#ifndef CPUCACHE_ARMV7M_H
#define CPUCACHE_ARMV7M_H

#include "cpucache-arm.h"
#include "cpu.h"

#define arm_dsb()           asm volatile("dsb" ::: "memory")
#define arm_isb()           asm volatile("isb" ::: "memory")

/* Discard entire icache and dcache, generally only used
 * when first enabling the caches. */
void __discard_idcache(void);

#endif /* CPUCACHE_ARMV7M_H */
