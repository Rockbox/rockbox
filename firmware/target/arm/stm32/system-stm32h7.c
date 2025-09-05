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
#include "gpio-stm32h7.h"
#include "regs/cortex-m/cm_scb.h"
#include "regs/cortex-m/cm_systick.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/pwr.h"
#include "regs/stm32h743/syscfg.h"
#include "regs/stm32h743/flash.h"
#include "regs/stm32h743/dbgmcu.h"

/* EXT timer is 1/8th of CPU clock */
#define SYSTICK_FREQ            (CPU_FREQ / 8)
#define SYSTICK_PER_MS          (SYSTICK_FREQ / 1000)
#define SYSTICK_PER_US          (SYSTICK_FREQ / 1000000)

/* Max delay is limited by kernel tick interval + safety margin */
#define SYSTICK_DELAY_MAX_US    (1000000 / HZ / 2)
#define SYSTICK_DELAY_MAX_MS    (SYSTICK_DELAY_MAX_US / 1000)

/* Flag to use VOS0 */
#define STM32H743_USE_VOS0      (CPU_FREQ > 400000000)

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

static void stm_init_hse(void)
{
    reg_writef(RCC_CR, HSEON(1));

    while (!reg_readf(RCC_CR, HSERDY));
}

static void stm_init_pll(void)
{
    /* TODO - this should be determined by the target in some way. */

    /* Select HSE/4 input for PLL1 (6 MHz) */
    reg_writef(RCC_PLLCKSELR,
               PLLSRC_V(HSE),
               DIVM1(4),
               DIVM2(0),
               DIVM3(0));

    /* Enable PLL1P and PLL1Q */
    reg_writef(RCC_PLLCFGR,
               DIVP1EN(1),
               DIVQ1EN(1),
               DIVR1EN(0),
               DIVP2EN(0),
               DIVQ2EN(0),
               DIVR2EN(0),
               DIVP3EN(0),
               DIVQ3EN(0),
               DIVR3EN(0),
               PLL1RGE_V(4_8MHZ),
               PLL1VCOSEL_V(WIDE));

    reg_writef(RCC_PLL1DIVR,
               DIVN(80 - 1), /* 6 * 80 = 480 MHz  */
               DIVP(1 - 1),  /* 480 / 1 = 480 MHz */
               DIVQ(8 - 1),  /* 480 / 8 = 60 MHz  */
               DIVR(1 - 1));

    reg_writef(RCC_CR, PLL1ON(1));
    while (!reg_readf(RCC_CR, PLL1RDY));
}

static void stm_init_vos(void)
{
    reg_writef(PWR_D3CR, VOS_V(VOS1));
    while (!reg_readf(PWR_D3CR, VOSRDY));

    if (STM32H743_USE_VOS0)
    {
        reg_writef(RCC_APB4ENR, SYSCFGEN(1));

        /* Set ODEN bit to enter VOS0 */
        reg_writef(SYSCFG_PWRCFG, ODEN(1));
        while (!reg_readf(PWR_D3CR, VOSRDY));

        reg_writef(RCC_APB4ENR, SYSCFGEN(0));
    }
}

static void stm_init_system_clock(void)
{
    /* Enable HCLK /2 divider (CPU is at 480 MHz, HCLK limit is 240 MHz) */
    reg_writef(RCC_D1CFGR, HPRE(8));
    while (reg_readf(RCC_D1CFGR, HPRE) != 8);

    /* Enable ABP /2 dividers (HCLK/2, APB limit is 120 MHz) */
    reg_writef(RCC_D1CFGR, D1PPRE(4));
    reg_writef(RCC_D2CFGR, D2PPRE1(4), D2PPRE2(4));
    reg_writef(RCC_D3CFGR, D3PPRE(4));

    /* Switch to PLL1P system clock source */
    reg_writef(RCC_CFGR, SW_V(PLL1P));
    while (reg_readf(RCC_CFGR, SWS) != BV_RCC_CFGR_SWS_PLL1P);

    /* Reduce flash access latency */
    reg_writef(FLASH_ACR, LATENCY(4), WRHIGHFREQ(2));
    while (reg_readf(FLASH_ACR, LATENCY) != 4);
}

static void stm_init_lse(void)
{
    /*
     * Skip if LSE and RTC are already enabled.
     */
    if (reg_readf(RCC_BDCR, LSERDY) &&
        reg_readf(RCC_BDCR, RTCEN))
        return;

    /*
     * Enable LSE and set it as RTC clock source,
     * then re-enable backup domain write protection.
     */

    reg_writef(PWR_CR1, DBP(1));

    /* Reset backup domain */
    reg_writef(RCC_BDCR, BDRST(1));
    reg_writef(RCC_BDCR, BDRST(0));

    reg_writef(RCC_BDCR, LSEON(1), LSEDRV(3));
    while (!reg_readf(RCC_BDCR, LSERDY));

    reg_writef(RCC_BDCR, RTCEN(1), RTCSEL_V(LSE));
    reg_writef(PWR_CR1, DBP(0));
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

    /* Start up clocks and get running at max CPU frequency */
    stm_init_hse();
    stm_init_pll();
    stm_init_vos();
    stm_init_system_clock();

    /* Start up LSE and RTC if necessary so backup domain works */
    stm_init_lse();

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
