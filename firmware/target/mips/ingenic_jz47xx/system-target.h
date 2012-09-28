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

/* no optimized byteswap functions implemented for mips, yet */
#define NEED_GENERIC_BYTESWAPS

/* static inline functions moved off jz4740.h */
/* PLL output frequency */
static __inline__ unsigned int __cpm_get_pllout(void)
{
    unsigned long m, n, no, pllout;
    unsigned long cppcr = REG_CPM_CPPCR;
    unsigned long od[4] = {1, 2, 2, 4};
    
    if ((cppcr & CPM_CPPCR_PLLEN) && !(cppcr & CPM_CPPCR_PLLBP))
    {
        m = __cpm_get_pllm() + 2;
        n = __cpm_get_plln() + 2;
        no = od[__cpm_get_pllod()];
        pllout = ((JZ_EXTAL) / (n * no)) * m;
    }
    else
        pllout = JZ_EXTAL;
    
    return pllout;
}

/* PLL output frequency for MSC/I2S/LCD/USB */
static __inline__ unsigned int __cpm_get_pllout2(void)
{
    if (REG_CPM_CPCCR & CPM_CPCCR_PCS)
        return __cpm_get_pllout();
    else
        return __cpm_get_pllout()/2;
}

/* CPU core clock */
static __inline__ unsigned int __cpm_get_cclk(void)
{
    int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

    return __cpm_get_pllout() / div[__cpm_get_cdiv()];
}

/* AHB system bus clock */
static __inline__ unsigned int __cpm_get_hclk(void)
{
    int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

    return __cpm_get_pllout() / div[__cpm_get_hdiv()];
}

/* Memory bus clock */
static __inline__ unsigned int __cpm_get_mclk(void)
{
    int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

    return __cpm_get_pllout() / div[__cpm_get_mdiv()];
}

/* APB peripheral bus clock */
static __inline__ unsigned int __cpm_get_pclk(void)
{
    int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

    return __cpm_get_pllout() / div[__cpm_get_pdiv()];
}

/* LCDC module clock */
static __inline__ unsigned int __cpm_get_lcdclk(void)
{
    return __cpm_get_pllout2() / (__cpm_get_ldiv() + 1);
}

/* LCD pixel clock */
static __inline__ unsigned int __cpm_get_pixclk(void)
{
    return __cpm_get_pllout2() / (__cpm_get_pixdiv() + 1);
}

/* I2S clock */
static __inline__ unsigned int __cpm_get_i2sclk(void)
{
    if (REG_CPM_CPCCR & CPM_CPCCR_I2CS)
        return __cpm_get_pllout2() / (__cpm_get_i2sdiv() + 1);
    else
        return JZ_EXTAL;
}

/* USB clock */
static __inline__ unsigned int __cpm_get_usbclk(void)
{
    if (REG_CPM_CPCCR & CPM_CPCCR_UCS)
        return __cpm_get_pllout2() / (__cpm_get_udiv() + 1);
    else
        return JZ_EXTAL;
}

/* MSC clock */
static __inline__ unsigned int __cpm_get_mscclk(void)
{
    return __cpm_get_pllout2() / (__cpm_get_mscdiv() + 1);
}

/* EXTAL clock for UART,I2C,SSI,TCU,USB-PHY */
static __inline__ unsigned int __cpm_get_extalclk(void)
{
    return JZ_EXTAL;
}

/* RTC clock for CPM,INTC,RTC,TCU,WDT */
static __inline__ unsigned int __cpm_get_rtcclk(void)
{
    return JZ_EXTAL2;
}

static __inline__ int __dmac_get_irq(void)
{
    int i;
    for (i = 0; i < MAX_DMA_NUM; i++)
        if (__dmac_channel_irq_detected(i))
            return i;
    return -1;
}

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

/*---------------------------------------------------------------------------
 * Put core in a power-saving state.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(void)
{
#if CONFIG_CPU == JZ4732
    __cpm_idle_mode();
#endif
    asm volatile(".set   mips32r2           \n"
                 "mfc0   $8, $12            \n" /* mfc t0, $12 */
                 "move   $9, $8             \n" /* move t1, t0 */
                 "la     $10, 0x8000000     \n" /* la t2, 0x8000000 */
                 "or     $8, $8, $10        \n" /* Enable reduced power mode */
                 "mtc0   $8, $12            \n" /* mtc t0, $12 */
                 "wait                      \n"
                 "mtc0   $9, $12            \n" /* mtc t1, $12 */
                 ".set   mips0              \n"
                 ::: "t0", "t1", "t2"
                 );
    enable_irq();
}


#endif /* __SYSTEM_TARGET_H_ */
