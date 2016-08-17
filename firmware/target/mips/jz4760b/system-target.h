/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 * Copyright (C) 2016 by Amaury Pouly
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

#ifndef __SYSTEM_TARGET_H_
#define __SYSTEM_TARGET_H_

#include <inttypes.h>

#include "config.h"
#include "jz4760b.h"
#include "mipsregs.h"

#define CACHE_SIZE       16*1024
#define CACHE_LINE_SIZE  32
#include "mmu-mips.h"

/* no optimized byteswap functions implemented for mips, yet */
#define NEED_GENERIC_BYTESWAPS

/* This one returns the old status */
static inline int set_interrupt_status(int status, int mask)
{
    unsigned int res, oldstatus;

    res = oldstatus = read_c0_status();
    res &= ~mask;
    res |= (status & mask);
    write_c0_status(res);

    return oldstatus;
}

static inline void enable_interrupt(void)
{
    /* Set IE bit */
    set_c0_status(ST0_IE);
}

static inline void disable_interrupt(void)
{
    /* Clear IE bit */
    clear_c0_status(ST0_IE);
}

static inline int disable_interrupt_save(int mask)
{
    return set_interrupt_status(0, mask);
}

static inline void restore_interrupt(int status)
{
    write_c0_status(status);
}

#define disable_irq() disable_interrupt()
#define enable_irq()  enable_interrupt()
#define HIGHEST_IRQ_LEVEL 0
#define set_irq_level(status)  set_interrupt_status((status), ST0_IE)
#define disable_irq_save()     disable_interrupt_save(ST0_IE)
#define restore_irq(c0_status) restore_interrupt(c0_status)

void udelay(unsigned int usec);

static inline void mdelay(unsigned msecs)
{
    udelay(1000 * msecs);
}

/*---------------------------------------------------------------------------
 * Put core in a power-saving state.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(void)
{
    /* TODO */
}


#endif /* __SYSTEM_TARGET_H_ */
