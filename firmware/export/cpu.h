/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
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
#ifndef __CPU_H
#define __CPU_H

#include "config.h"

#if CONFIG_CPU == MCF5249
#include "mcf5249.h"
#elif CONFIG_CPU == MCF5250
#include "mcf5250.h"
#elif (CONFIG_CPU == PP5020) || (CONFIG_CPU == PP5022)
#include "pp5020.h"
#elif CONFIG_CPU == PP5002
#include "pp5002.h"
#elif CONFIG_CPU == PP5024
#include "pp5024.h"
#elif CONFIG_CPU == PP6100
#include "pp6100.h"
#elif CONFIG_CPU == S3C2440
#include "s3c2440.h"
#elif CONFIG_CPU == DM320
#include "dm320.h"
#elif CONFIG_CPU == IMX31L
#include "imx31l.h"
#elif defined(CPU_TCC780X)
#include "tcc780x.h"
#elif defined(CPU_S5L87XX)
#include "s5l87xx.h"
#elif CONFIG_CPU == JZ4732
#include "jz4740.h"
#elif CONFIG_CPU == JZ4760B
#include "jz4760b.h"
#elif CONFIG_CPU == AS3525
#include "as3525.h"
#elif CONFIG_CPU == AS3525v2
#include "as3525v2.h"
#elif CONFIG_CPU == IMX233
#include "imx233.h"
#elif CONFIG_CPU == RK27XX
#include "rk27xx.h"
#elif CONFIG_CPU == X1000
#include "x1000.h"
#elif CONFIG_CPU == STM32H743
#include "stm32h743.h"
#endif

#if (CONFIG_PLATFORM & PLATFORM_NATIVE) && (defined(CPU_ARM) || defined(CPU_MIPS))
# define HAVE_CPU_CACHE_ALIGN
#endif

#if defined(HAVE_CPU_CACHE_ALIGN)
# if !defined(CACHEALIGN_BITS)
#  error "CPU header must define CACHEALIGN_BITS"
# elif !defined(CACHEALIGN_SIZE)
#  error "CPU header must define CACHEALIGN_SIZE"
# elif CACHEALIGN_SIZE != (1u << CACHEALIGN_BITS)
#  error "CACHEALIGN_SIZE and CACHEALIGN_BITS are inconsistent"
# endif
#else
# if defined(CACHEALIGN_BITS) && defined(CACHEALIGN_SIZE)
#  error "CACHEALIGN_BITS and CACHEALIGN_SIZE must not be defined for targets with no CPU cache"
# endif
#endif

/*
 * Note: NOCACHE_BASE assumes that DRAM is linearly mapped both
 * at a lower cached address and an upper uncached address, so
 * that you can add NOCACHE_BASE to the cached DRAM address to
 * get the corresponding uncached address.
 *
 * Defining NOCACHE_BASE is only required if you need plugins to
 * be able to link data at uncached addresses. If in doubt, you
 * don't need this. It's mainly of use for dual-core PortalPlayer
 * targets which need to do this for things like mutexes/queues;
 * since PP lacks hardware cache coherency, data which is writable
 * by more than one core often needs to accessed uncached.
 */
#if defined(NOCACHE_BASE)
# if !defined(HAVE_CPU_CACHE_ALIGN)
#  error "NOCACHE_BASE cannot be defined on targets with no CPU cache!"
# elif NOCACHE_BASE == 0
#  error "NOCACHE_BASE cannot be 0!"
# endif
#endif

#endif /* __CPU_H */
