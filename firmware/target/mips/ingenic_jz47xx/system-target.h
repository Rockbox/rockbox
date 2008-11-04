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

#ifndef __SYSTEM_TARGET_H_
#define __SYSTEM_TARGET_H_

#include "config.h"
#include "jz4740.h"
#include "mipsregs.h"

/* This one returns the old status */
#define HIGHEST_IRQ_LEVEL 0

#define set_irq_level(status) \
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

static inline int disable_interrupt_save(int mask)
{
	unsigned int oldstatus;
    
	oldstatus = read_c0_status();
	write_c0_status(oldstatus | mask);
    
	return oldstatus;
}

#define disable_irq_save() \
    disable_interrupt_save(ST0_IE)

static inline void restore_interrupt(int status)
{
    write_c0_status(status);
}

#define restore_irq(c0_status) \
    restore_interrupt(c0_status)
    
#define	swap16(x) (((x) & 0xff) << 8 | ((x) >> 8) & 0xff)
#define	swap32(x) (((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) >> 24) & 0xff)

#define UNCACHED_ADDRESS(addr)    ((unsigned int)(addr) | 0xA0000000)
#define PHYSADDR(x)               ((x) & 0x1fffffff)

void __dcache_writeback_all(void);
void __dcache_invalidate_all(void);
void __icache_invalidate_all(void);
void __flush_dcache_line(unsigned long addr);
void dma_cache_wback_inv(unsigned long addr, unsigned long size);
void system_enable_irq(unsigned int irq);
void sti(void);
void cli(void);
void udelay(unsigned int usec);
void mdelay(unsigned int msec);
void power_off(void);
void system_reboot(void);

#define DMA_LCD_CHANNEL    0
#define DMA_NAND_CHANNEL   1
#define DMA_USB_CHANNEL    2
#define DMA_AIC_TX_CHANNEL 3

#endif /* __SYSTEM_TARGET_H_ */
