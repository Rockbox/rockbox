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
 
#include "config.h"
#include "jz4740.h"
#include "mipsregs.h"

/* Core-level interrupt masking */

/* This one returns the old status */
#define HIGHEST_IRQ_LEVEL 0

#define set_irq_level(status) \
    set_interrupt_status((status), ST0_IE)
#define set_fiq_status(status) \
    set_interrupt_status((status), ST0_IE)

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

#define disable_irq() \
    disable_interrupt()

#define enable_irq() \
    enable_interrupt()

#define disable_fiq() \
    disable_interrupt()

#define enable_fiq() \
    enable_interrupt()

static inline int disable_interrupt_save(int mask)
{
	unsigned int oldstatus;
    
	oldstatus = read_c0_status();
	write_c0_status(oldstatus | mask);
    
	return oldstatus;
}

#define disable_irq_save() \
    disable_interrupt_save(ST0_IE)

#define disable_fiq_save() \
    disable_interrupt_save(ST0_IE)

static inline void restore_interrupt(int status)
{
    write_c0_status(status);
}

#define restore_irq(cpsr) \
    restore_interrupt(cpsr)

#define restore_fiq(cpsr) \
    restore_interrupt(cpsr)
    
#define	swap16(x) (((x) & 0xff) << 8 | ((x) >> 8) & 0xff)
#define	swap32(x) (((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) >> 24) & 0xff)

void sti(void);
void cli(void);
