/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Tomasz Malesinski
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

#include <stdlib.h>
#include "pnx0101.h"
#include "system.h"

static struct
{
  unsigned char freq;
  unsigned char sys_mult;
  unsigned char sys_div;
}
perf_modes[3] ICONST_ATTR =
{
    {12, 4, 4},
    {48, 4, 1},
    {60, 5, 1}
};

static int performance_mode, bus_divider;

static void cgu_set_sel_stage_input(int clock, int input)
{
    int s = CGU.base_ssr[clock];
    if (s & 1)
        CGU.base_fs2[clock] = input;
    else
        CGU.base_fs1[clock] = input;
    CGU.base_scr[clock] = (s & 3) ^ 3;
}

static void cgu_reset_sel_stage_clocks(int first_esr, int n_esr,
                                       int first_div, int n_div)
{
    int i;
    for (i = 0; i < n_esr; i++)
        CGU.clk_esr[first_esr + i] = 0;
    for (i = 0; i < n_div; i++)
        CGU.base_fdc[first_div + i] = 0;
}

static void cgu_configure_div(int div, int n, int m)
{
    int msub, madd, div_size, max_n;
    unsigned long cfg;
    
    if (n == m)
    {
        CGU.base_fdc[div] = CGU.base_fdc[div] & ~1;
        return;
    }
    
    msub = -n;
    madd = m - n;
    div_size = (div == PNX0101_HIPREC_FDC) ? 10 : 8;
    max_n = 1 << div_size;
    while ((madd << 1) < max_n && (msub << 1) >= -max_n)
    {
        madd <<= 1;
        msub <<= 1;
    }
    cfg = (((msub << div_size) | madd) << 3) | 4;
    CGU.base_fdc[div] = CGU.base_fdc[div] & ~1;
    CGU.base_fdc[div] = cfg | 2;
    CGU.base_fdc[div] = cfg;
    CGU.base_fdc[div] = cfg | 1;
}

static void cgu_connect_div_to_clock(int rel_div, int esr)
{
    CGU.clk_esr[esr] = (rel_div << 1) | 1;
}

static void cgu_enable_clock(int clock)
{
    CGU.clk_pcr[clock] |= 1;
}

static void cgu_start_sel_stage_dividers(int bcr)
{
    CGU.base_bcr[bcr] = 1;
}

/* Convert a pointer that points to IRAM (0x4xxxx) to a pointer that
   points to the uncached page (0x0xxxx) that is also mapped to IRAM. */
static inline void *noncached(void *p)
{
    return (void *)(((unsigned long)p) & 0xffff);
}

/* To avoid SRAM accesses while changing memory controller settings we
   run this routine from uncached copy of IRAM. All times are in CPU
   cycles. At CPU frequencies lower than 60 MHz we could use faster
   settings, but since DMA may access SRAM at any time, changing
   memory timings together with CPU frequency would be tricky. */
static void do_set_mem_timings(void) ICODE_ATTR;
static void do_set_mem_timings(void)
{
    int old_irq = disable_irq_save();
    while ((EMC.status & 3) != 0);
    EMC.control = 5;
    EMCSTATIC0.waitrd = 6;
    EMCSTATIC0.waitwr = 5;
    EMCSTATIC1.waitrd = 5;
    EMCSTATIC1.waitwr = 4; /* OF uses 5 here */
    EMCSTATIC2.waitrd = 4;
    EMCSTATIC2.waitwr = 3;
    EMCSTATIC0.waitoen = 1;
    EMCSTATIC1.waitoen = 1;
    EMCSTATIC2.waitoen = 1;
    /* Enable write buffers for SRAM. */
#ifndef DEBUG
    EMCSTATIC1.config = 0x80081;
#endif
    EMC.control = 1;
    restore_irq(old_irq);
}

static void emc_set_mem_timings(void)
{
    void (*f)(void) = noncached(do_set_mem_timings);
    (*f)();
}

static void cgu_set_sys_mult(int i)
{
    cgu_set_sel_stage_input(PNX0101_SEL_STAGE_SYS, PNX0101_MAIN_CLOCK_FAST);
    cgu_set_sel_stage_input(PNX0101_SEL_STAGE_APB3, PNX0101_MAIN_CLOCK_FAST);

    PLL.lppdn = 1;
    PLL.lpfin = 1;
    PLL.lpmbyp = 0;
    PLL.lpdbyp = 0;
    PLL.lppsel = 1;
    PLL.lpmsel  = i - 1;
    PLL.lppdn = 0;
    while (!PLL.lplock);

    cgu_configure_div(PNX0101_FIRST_DIV_SYS + 1, 1, (i == 5) ? 15 : 12);
    cgu_connect_div_to_clock(1, 0x11);
    cgu_enable_clock(0x11);
    cgu_start_sel_stage_dividers(PNX0101_BCR_SYS);

    cgu_set_sel_stage_input(PNX0101_SEL_STAGE_SYS,
                            PNX0101_MAIN_CLOCK_MAIN_PLL);
    cgu_set_sel_stage_input(PNX0101_SEL_STAGE_APB3,
                            PNX0101_MAIN_CLOCK_MAIN_PLL);
}

static void pnx0101_set_performance_mode(int mode)
{
    int old = performance_mode;
    if (perf_modes[old].sys_mult != perf_modes[mode].sys_mult)
        cgu_set_sys_mult(perf_modes[mode].sys_mult);
    if (perf_modes[old].sys_div != perf_modes[mode].sys_div)
        cgu_configure_div(bus_divider, 1, perf_modes[mode].sys_div);
    performance_mode = mode;
}

static void pnx0101_init_clocks(void)
{
    bus_divider = PNX0101_FIRST_DIV_SYS + (CGU.clk_esr[0] >> 1);
    performance_mode = 0;
    emc_set_mem_timings();
    pnx0101_set_performance_mode(2);

    cgu_set_sel_stage_input(PNX0101_SEL_STAGE_APB1,
                            PNX0101_MAIN_CLOCK_FAST);
    cgu_reset_sel_stage_clocks(PNX0101_FIRST_ESR_APB1, PNX0101_N_ESR_APB1,
                               PNX0101_FIRST_DIV_APB1, PNX0101_N_DIV_APB1);
    cgu_configure_div(PNX0101_FIRST_DIV_APB1, 1, 4);
    cgu_connect_div_to_clock(0, PNX0101_ESR_APB1);
    cgu_connect_div_to_clock(0, PNX0101_ESR_T0);
    cgu_connect_div_to_clock(0, PNX0101_ESR_T1);
    cgu_connect_div_to_clock(0, PNX0101_ESR_I2C);
    cgu_enable_clock(PNX0101_CLOCK_APB1);
    cgu_enable_clock(PNX0101_CLOCK_T0);
    cgu_enable_clock(PNX0101_CLOCK_T1);
    cgu_enable_clock(PNX0101_CLOCK_I2C);
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    switch (frequency)
    {
        case CPUFREQ_MAX:
            pnx0101_set_performance_mode(2);
            cpu_frequency = CPUFREQ_MAX;
            break;
        case CPUFREQ_NORMAL:
            pnx0101_set_performance_mode(1);
            cpu_frequency = CPUFREQ_NORMAL;
            break;
        case CPUFREQ_DEFAULT:
        default:
            pnx0101_set_performance_mode(0);
            cpu_frequency = CPUFREQ_DEFAULT;
            break;
    }
    
}
#endif

interrupt_handler_t interrupt_vector[0x1d]  __attribute__ ((section(".idata")));

#define IRQ_READ(reg, dest) \
    do { unsigned long v2; \
        do { \
            dest = (reg); \
            v2 = (reg); \
        } while ((dest != v2)); \
    } while (0);

#define IRQ_WRITE_WAIT(reg, val, cond) \
    do { unsigned long v, v2; \
        do { \
            (reg) = (val); \
            v = (reg); \
            v2 = (reg); \
        } while ((v != v2) || !(cond)); \
    } while (0);

static void undefined_int(void)
{
}

void irq(void)
{
    unsigned long n;
    IRQ_READ(INTVECTOR[0], n)
    (*(interrupt_vector[n >> 3]))();
}

void fiq(void)
{
}

void irq_enable_int(int n)
{
    IRQ_WRITE_WAIT(INTREQ[n], INTREQ_WEENABLE | INTREQ_ENABLE, v & 0x10000);
}

void irq_disable_int(int n)
{
    IRQ_WRITE_WAIT(INTREQ[n], INTREQ_WEENABLE, (v & 0x10000) == 0);
}

void irq_set_int_handler(int n, interrupt_handler_t handler)
{
    interrupt_vector[n] = handler;
}

void system_init(void)
{
    int i;

    /* turn off watchdog */
    (*(volatile unsigned long *)0x80002804) = 0;

    /*
    IRQ_WRITE_WAIT(INTVECTOR[0], 0, v == 0);
    IRQ_WRITE_WAIT(INTVECTOR[1], 0, v == 0);
    IRQ_WRITE_WAIT(INTPRIOMASK[0], 0, v == 0);
    IRQ_WRITE_WAIT(INTPRIOMASK[1], 0, v == 0);
    */

    for (i = 1; i <= 0x1c; i++)
    {
        IRQ_WRITE_WAIT(INTREQ[i],
                       INTREQ_WEPRIO | INTREQ_WETARGET |
                       INTREQ_WEENABLE | INTREQ_WEACTVLO | 1,
                       (v & 0x3010f) == 1);
        IRQ_WRITE_WAIT(INTREQ[i], INTREQ_WEENABLE, (v & 0x10000) == 0);
        IRQ_WRITE_WAIT(INTREQ[i], INTREQ_WEPRIO | 1, (v & 0xf) == 1);
        interrupt_vector[i] = undefined_int;
    }
    interrupt_vector[0] = undefined_int;
    pnx0101_init_clocks();
}


void system_reboot(void)
{
    (*(volatile unsigned long *)0x80002804) = 1;
    while (1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
