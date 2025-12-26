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
#include "clock-stm32h7.h"
#include "gpio-stm32h7.h"
#include "regs/cortex-m/cm_scb.h"
#include "regs/cortex-m/cm_systick.h"
#include "regs/stm32h743/dbgmcu.h"

/* EXT timer is 1/8th of CPU clock */
#define SYSTICK_FREQ            (CPU_FREQ / 8)
#define SYSTICK_PER_MS          (SYSTICK_FREQ / 1000)
#define SYSTICK_PER_US          (SYSTICK_FREQ / 1000000)

/* Max delay is limited by kernel tick interval + safety margin */
#define SYSTICK_DELAY_MAX_US    (1000000 / HZ / 2)
#define SYSTICK_DELAY_MAX_MS    (SYSTICK_DELAY_MAX_US / 1000)

/* Base address of vector table */
extern char __vectors_arm[];

static void systick_init(unsigned int interval_in_ms)
{
    reg_writef(CM_SYSTICK_RVR, VALUE(SYSTICK_PER_MS * interval_in_ms - 1));
    reg_writef(CM_SYSTICK_CVR, VALUE(0));
    reg_writef(CM_SYSTICK_CSR, CLKSOURCE_V(EXT), ENABLE(1));
}

static void stm_enable_caches(void)
{
    __discard_idcache();

    reg_writef(CM_SCB_CCR, IC(1), DC(1));

    arm_dsb();
    arm_isb();
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
    stm_enable_caches();

    /* Initialize system clocks */
    stm_target_clock_init();

    /* TODO: move this */
    systick_init(1000/HZ);

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
    (void)interval_in_ms;

    reg_writef(CM_SYSTICK_CSR, TICKINT(1));
}

void systick_handler(void)
{
    call_tick_tasks();
}

/*
 * NOTE: This assumes that the CPU cannot be reclocked during an interrupt.
 * If that happens, the systick interval and reload value would be modified
 * to maintain the kernel tick interval and the code here will break.
 */
static void __udelay(uint32_t us)
{
    uint32_t start = reg_readf(CM_SYSTICK_CVR, VALUE);
    uint32_t max = reg_readf(CM_SYSTICK_RVR, VALUE);
    uint32_t delay = us * SYSTICK_PER_US;

    for (;;)
    {
        uint32_t value = reg_readf(CM_SYSTICK_CVR, VALUE);
        uint32_t diff = start - value;
        if (value > start)
            diff += max;
        if (diff >= delay)
            break;
    }
}

void udelay(uint32_t us)
{
    while (us > SYSTICK_DELAY_MAX_US)
    {
        __udelay(SYSTICK_DELAY_MAX_US);
        us -= SYSTICK_DELAY_MAX_US;
    }

    __udelay(us);
}

void mdelay(uint32_t ms)
{
    while (ms > SYSTICK_DELAY_MAX_MS)
    {
        __udelay(SYSTICK_DELAY_MAX_MS * 1000);
        ms -= SYSTICK_DELAY_MAX_MS;
    }

    __udelay(ms * 1000);
}

void system_exception_wait(void)
{
    while (1);
}

int system_memory_guard(int newmode)
{
    /* TODO -- maybe use MPU here to give some basic protection */
    (void)newmode;
    return MEMGUARD_NONE;
}
