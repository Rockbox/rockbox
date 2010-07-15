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

#include <inttypes.h>

#include "config.h"
#include "jz4740.h"
#include "mipsregs.h"

#define CACHE_SIZE       16*1024
#define CACHE_LINE_SIZE  32
#include "mmu-mips.h"

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

static inline uint16_t swap16(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t hi = swap16(value >> 16);
    uint32_t lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

#define UNCACHED_ADDRESS(addr)    ((unsigned int)(addr) | 0xA0000000)
#define UNCACHED_ADDR(x)          UNCACHED_ADDRESS((x))
#define PHYSADDR(x)               ((x) & 0x1fffffff)

void system_enable_irq(unsigned int irq);
void udelay(unsigned int usec);
void mdelay(unsigned int msec);
void dma_enable(void);
void dma_disable(void);

#define DMA_AIC_TX_CHANNEL 0
#define DMA_NAND_CHANNEL   1
#define DMA_USB_CHANNEL    2
#define DMA_LCD_CHANNEL    3

#define XDMA_CALLBACK(n) DMA ## n
#define DMA_CALLBACK(n)  XDMA_CALLBACK(n)

#define DMA_IRQ(n)      (IRQ_DMA_0 + (n))
#define GPIO_IRQ(n)     (IRQ_GPIO_0 + (n))

#endif /* __SYSTEM_TARGET_H_ */
