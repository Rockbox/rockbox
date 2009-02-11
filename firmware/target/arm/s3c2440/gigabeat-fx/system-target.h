/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Greg White
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
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"

#define CPUFREQ_DEFAULT 98784000
#define CPUFREQ_NORMAL  98784000
#define CPUFREQ_MAX    296352000

void system_prepare_fw_start(void);
void tick_stop(void);

/* Functions to set and clear regiser bits atomically */

/* Set and clear register bits */
void s3c_regmod32(volatile unsigned long *reg, unsigned long bits,
                  unsigned long mask);
/* Set register bits */
void s3c_regset32(volatile unsigned long *reg, unsigned long bits);
/* Clear register bits */
void s3c_regclr32(volatile unsigned long *reg, unsigned long bits);

#define HAVE_CPUCACHE_FLUSH
#define HAVE_CPUCACHE_INVALIDATE
#define cpucache_flush      clean_dcache
#define cpucache_invalidate invalidate_idcache

#endif /* SYSTEM_TARGET_H */
