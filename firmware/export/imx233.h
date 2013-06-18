/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __IMX233_H__
#define __IMX233_H__

#ifndef IMX233_SUBTARGET
#error You must define IMX233_SUBTARGET to select the chip family
#endif

#ifndef IMX233_PACKAGE
#error You must IMX233_PACKAGE to select the chip package
#endif

/*
 * Chip Memory Map (stmp3700,imx233):
 *   0x00000000 - 0x00007fff: on chip ram
 *   0x40000000 - 0x5fffffff: dram (512Mb max)
 *   0x80000000 - 0x80100000: memory mapped registers
 * We use the following map:
 *   0x60000000 - 0x7fffffff: dram (cached)
 *   0x90000000 - 0xafffffff: dram (buffered)
 *   everything else        : identity mapped (uncached)
 *
 * Chip Memory Map (stmp3600):
 *   0x00000000 - 0x00007fff: on chip ram
 *   0x60000000 - 0x7fffffff: dram (512Mb max)
 *   0x80000000 - 0x80100000: memory mapped registers
 * We use the following map:
 *   0x40000000 - 0x5fffffff: dram (cached)
 *   0x90000000 - 0xafffffff: dram (buffered)
 *   everything else        : identity mapped (uncached)
 *
 * As a side note it's important to notice that uncached dram is identity mapped
 */

#define IRAM_ORIG           0
#if IMX233_SUBTARGET >= 3780
#define IRAM_SIZE           (32 * 1024)
#elif IMX233_SUBTARGET >= 3770
#define IRAM_SIZE           (512 * 1024)
#else
#define IRAM_SIZE           (256 * 1024)
#endif

#if IMX233_SUBTARGET >= 3700
#define DRAM_ORIG           0x40000000
#else
#define DRAM_ORIG           0x60000000
#endif
#define DRAM_SIZE           (MEMORYSIZE * 0x100000)

#if IMX233_SUBTARGET >= 3700
#define UNCACHED_DRAM_ADDR  0x40000000
#define CACHED_DRAM_ADDR    0x60000000
#define BUFFERED_DRAM_ADDR  0x90000000
#else
#define UNCACHED_DRAM_ADDR  0x60000000
#define CACHED_DRAM_ADDR    0x40000000
#define BUFFERED_DRAM_ADDR  0x90000000
#endif

/* 32 bytes per cache line */
#define CACHEALIGN_SIZE     32
#define CACHEALIGN_BITS     5

#define NOCACHE_BASE        (UNCACHED_DRAM_ADDR - CACHED_DRAM_ADDR)

#define __IN_RANGE(type, a) (type##_DRAM_ADDR <= (a) && (a) < (type##_DRAM_ADDR + DRAM_SIZE))
#define PHYSICAL_ADDR(a) \
    ((typeof(a))(__IN_RANGE(BUFFERED, (uintptr_t)(a)) ? \
        ((uintptr_t)(a) - BUFFERED_DRAM_ADDR + UNCACHED_DRAM_ADDR) \
        :__IN_RANGE(CACHED, (uintptr_t)(a)) ? \
        ((uintptr_t)(a) - CACHED_DRAM_ADDR + UNCACHED_DRAM_ADDR) \
        :(uintptr_t)(a)))
#define UNCACHED_ADDR(a) PHYSICAL_ADDR(a)

#define TTB_BASE_ADDR   (DRAM_ORIG + DRAM_SIZE - TTB_SIZE)
#define TTB_SIZE        0x4000
#define TTB_BASE        ((unsigned long *)TTB_BASE_ADDR)
/* align to cache line */
#ifndef IMX233_FRAMEBUFFER_SIZE
#define IMX233_FRAMEBUFFER_SIZE (LCD_WIDTH * LCD_HEIGHT * LCD_DEPTH / 8)
#endif
#define FRAME_SIZE      ((IMX233_FRAMEBUFFER_SIZE + CACHEALIGN_SIZE - 1) & ~(CACHEALIGN_SIZE - 1))
#define FRAME_PHYS_ADDR (DRAM_ORIG + DRAM_SIZE - TTB_SIZE - FRAME_SIZE)
#define FRAME           ((void *)(FRAME_PHYS_ADDR - UNCACHED_DRAM_ADDR + BUFFERED_DRAM_ADDR))

/* Timer runs at 32KHz, derived from clk32k@32KHz */
#define TIMER_FREQ      32000

/* USBOTG */
#define USB_QHARRAY_ATTR    __attribute__((section(".qharray"),nocommon,aligned(2048)))
#define USB_NUM_ENDPOINTS   5
/* STMP3600 doesn't have the bandwidth to put buffer in SDRAM */
#if IMX233_SUBTARGET < 3700
#define USB_DEVBSS_ATTR     IBSS_ATTR
#else
#define USB_DEVBSS_ATTR     NOCACHEBSS_ATTR
#endif
#define USB_BASE            0x80080000

#define ___ENSURE_ZERO(line, x) static uint8_t __ensure_zero_##line[-(x)] __attribute__((unused));
#define __ENSURE_ZERO(x) ___ENSURE_ZERO(__LINE__, x)
#define __ENSURE_MULTIPLE(x, y) __ENSURE_ZERO((x) % (y))
#define __ENSURE_CACHELINE_MULTIPLE(x) __ENSURE_MULTIPLE(x, 1 << CACHEALIGN_BITS)
#define __ENSURE_STRUCT_CACHE_FRIENDLY(name) __ENSURE_CACHELINE_MULTIPLE(sizeof(name))

#define __REG_SET(reg)  (*((volatile uint32_t *)(&reg + 1)))
#define __REG_CLR(reg)  (*((volatile uint32_t *)(&reg + 2)))
#define __REG_TOG(reg)  (*((volatile uint32_t *)(&reg + 3)))
#define __REG_SET_CLR(reg, set) \
    (*((volatile uint32_t *)(&reg + (set ? 1 : 2))))

#define __BLOCK_SFTRST  (1 << 31)
#define __BLOCK_CLKGATE (1 << 30)

#define __XTRACT(reg, field)    ((reg & reg##__##field##_BM) >> reg##__##field##_BP)
#define __XTRACT_EX(val, field)    (((val) & field##_BM) >> field##_BP)
#define __FIELD_SET(reg, field, val) reg = (reg & ~reg##__##field##_BM) | (val << reg##__##field##_BP)
#define __FIELD_SET_CLR(reg, field, set) __REG_SET_CLR(reg, set) = reg##__##field

/**
 * Register Naming Scheme
 *
 * => Devices
 *
 * Each device <dev> has its base address defined as REGS_<dev>_base:
 * 
 * Example:
 *  #define REGS_APBHBASE (0x80004000)
 *  #define REGS_SSPBASE(i) ((i) == 1 ? 0x80010000 : 0x80034000)
 *
 * => Registers
 *
 * Each register <reg> in device <dev> has its address(es) defined as
 * HW_<dev>_<reg>[_{SET,CLR,TOG}]
 * 
 * Examples:
 *  #define HW_APBH_CTRL1 (*(volatile unsigned long *)(REGS_APBHBASE + 0x10 + 0))
 *  #define HW_APBH_CHn_CURCMDAR(n) (*(volatile unsigned long *)(REGS_APBHBASE + 0x40+(n)*0x70))
 *  #define HW_SSP_CTRL0_SET(d) (*(volatile unsigned long *)(REGS_SSPBASE(d) + 0 + 0x4))
 *
 * => Fields
 *
 * Each field <field> in register <reg> in device <dev> has its bit position
 * and bitmask defined as {BP,BM}_<dev>_<reg>_<field>
 *
 * Examples:
 * 
 * 
 * 
 */

/**
 * Register macros:
 *
 * BF_SET(reg, field): equivalent to HW_reg_SET = BM_reg_field;
 * BF_CLR(reg, field): same with CLR
 * BF_TOG(reg, field): same with TOG
 *
 * BF_SETV(reg, field, v): equivalent to HW_reg_SET = BF_reg_field(v)
 * BF_CLRV(reg, field, v): same with CLR
 * BF_TOGV(reg, fielf, v): same with TOG
 *
 * BF_RD(reg, field): equivalent to (HW_reg & BM_reg_field) >> BP_reg_field
 * BF_WR(reg, field, v): equivalent to HW_reg = (HW_reg & ~BM_reg_field) | (((v << BP_reg_field) & BM_reg_field)
 * BF_WR_V(reg, field, sym): BF_WR(reg, field, BV_reg_field__sym)
 *
 * BF_{SET,CLR,TOG}[V]n(reg, n, field): same for multi registers
 *
 * The BF_RDX(val, reg, field) reads from the value provided instead of the register
 * Similarly for BF_WRX
 * 
 */

/**
 * Handy macros for mutliple operations at once
 *
 * BF_ORp(reg, f1,, ..., fp) is equivalent to
 * BF_reg_f1 | ... | BF_reg_fp
 *
 * BM_ORp is similar with BM_
 *
 * There exist some variadic variants which do not need to write the number
 * of parameters, if supported by the compiler:
 *
 * BF_OR(reg, f1, ..., fn)
 * BM_OR(reg, f1, ..., fn)
 */

#endif /* __IMX233_H__ */
