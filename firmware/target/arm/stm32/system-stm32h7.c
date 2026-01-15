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
#include "system.h"
#include "tick.h"
#include "button.h"
#include "clock-stm32h7.h"
#include "gpio-stm32h7.h"
#include "regs/cortex-m/cm_scb.h"
#include "regs/cortex-m/cm_systick.h"
#include "regs/stm32h743/dbgmcu.h"

/* Assumed initial CPU frequency for calculating systick */
#ifndef CPUFREQ_INITIAL
# define CPUFREQ_INITIAL CPU_FREQ
#endif

/* Tick interval in milliseconds */
#ifndef SYSTICK_INTERVAL_INITIAL
# define SYSTICK_INTERVAL_INITIAL (1000 / HZ)
#endif

/* Use EXT source which is equal to CPU frequency divided by 8 */
#define SYSTICK_SOURCE    BV_CM_SYSTICK_CSR_CLKSOURCE_EXT
#define SYSTICK_PRESCALER 8

/* Convert CPU frequency to number of systick ticks in 1 ms */
#define CPUFREQ_TO_SYSTICK_PER_MS(f) \
    ((f) / (SYSTICK_PRESCALER * 1000))

/* SysTick related state */
static uint32_t systick_per_ms = CPUFREQ_TO_SYSTICK_PER_MS(CPUFREQ_INITIAL);
static uint32_t systick_interval_in_ms = SYSTICK_INTERVAL_INITIAL;

/* Base address of vector table */
extern char __vectors_arm[];

void stm32_enable_caches(void)
{
    __discard_idcache();

    reg_writef(CM_SCB_CCR, IC(1), DC(1));

    arm_dsb();
    arm_isb();
}

static void stm32_recalc_systick_rvr(void)
{
    uint32_t ticks = systick_per_ms * systick_interval_in_ms;

    reg_writef(CM_SYSTICK_RVR, VALUE(ticks - 1));
}

static void stm32_set_systick_interval(uint32_t interval_in_ms)
{
    if (interval_in_ms != systick_interval_in_ms)
    {
        systick_interval_in_ms = interval_in_ms;
        stm32_recalc_systick_rvr();
    }
}

void stm32_systick_set_cpu_freq(uint32_t freq)
{
    uint32_t ticks_per_ms = CPUFREQ_TO_SYSTICK_PER_MS(freq);

    if (ticks_per_ms != systick_per_ms)
    {
        systick_per_ms = ticks_per_ms;
        stm32_recalc_systick_rvr();
    }
}

void stm32_systick_enable(void)
{
    stm32_recalc_systick_rvr();
    reg_writef(CM_SYSTICK_CVR, VALUE(0));
    reg_writef(CM_SYSTICK_CSR, CLKSOURCE(SYSTICK_SOURCE), ENABLE(1));
}

void stm32_systick_disable(void)
{
    reg_writef(CM_SYSTICK_CSR, ENABLE(0), TICKINT(0));
}

void system_init(void)
{
#if defined(DEBUG)
    system_debug_enable(true);
#endif

    /* Ensure IRQs are disabled and set vector table address */
    disable_irq();
    reg_var(CM_SCB_VTOR) = (uint32_t)__vectors_arm;

    /* Enable CPU caches */
    stm32_enable_caches();

    /* Initialize system clocks */
    stm_clock_init();

    /* Initialize systick */
    stm32_systick_enable();

    /* Call target-specific initialization */
    gpio_init();
    fmc_init();
}

void system_debug_enable(bool enable)
{
    /*
     * Debug infrastructure is split across D3 and D1 power domains and
     * normally these domains can be automatically clock gated when the
     * CPU enters sleep mode. Because of this, the debugger might not be
     * able to properly attach if the CPU is in sleep mode.
     *
     * Overriding clock gating using DBGMCU_CR makes debugger attaches
     * more reliable, although it will increase power consumption.
     */
    reg_writef(DBGMCU_CR,
               D1DBGCKEN(enable),
               D3DBGCKEN(enable),
               DBGSLEEP_D1(enable));
}

void tick_start(unsigned int interval_in_ms)
{
    stm32_set_systick_interval(interval_in_ms);
    stm32_systick_enable();

    reg_writef(CM_SYSTICK_CSR, TICKINT(1));
}

void systick_handler(void)
{
    call_tick_tasks();
}

/*
 * This makes two assumptions:
 *
 * 1. the CPU frequency must not change while udelay() is running;
 *    otherwise the delay time will be wrong.
 * 2. interrupt handlers should not block execution for more than
 *    one systick interval; if this happens the delay may be much
 *    longer than necessary.
 */
void udelay(uint32_t us)
{
    uint32_t delay_ticks = (us * systick_per_ms / 1000);
    uint32_t start = reg_readf(CM_SYSTICK_CVR, VALUE);
    uint32_t max = reg_readf(CM_SYSTICK_RVR, VALUE);

    while (delay_ticks > 0)
    {
        uint32_t value = reg_readf(CM_SYSTICK_CVR, VALUE);
        uint32_t diff = start - value;
        if (value > start)
            diff += max;

        if (diff >= delay_ticks)
            break;

        delay_ticks -= diff;
        start = value;
    }
}

void system_exception_wait(void)
{
#if defined(ECHO_R1)
    while (button_read_device() != (BUTTON_POWER | BUTTON_START));
#else
    while (1);
#endif
}

int system_memory_guard(int newmode)
{
    /* TODO -- maybe use MPU here to give some basic protection */
    (void)newmode;
    return MEMGUARD_NONE;
}
