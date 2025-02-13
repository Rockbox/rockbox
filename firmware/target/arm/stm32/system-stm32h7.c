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
#include "cortex-m/scb.h"
#include "cortex-m/systick.h"
#include "stm32h7/rcc.h"
#include "stm32h7/pwr.h"
#include "stm32h7/syscfg.h"
#include "stm32h7/flash.h"

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
    cm_writef(SYSTICK_RVR, VALUE(SYSTICK_PER_MS * interval_in_ms - 1));
    cm_writef(SYSTICK_CVR, VALUE(0));
    cm_writef(SYSTICK_CSR, CLKSOURCE_V(EXT), ENABLE(1));
}

static void stm_enable_caches(void)
{
    __discard_idcache();

    cm_writef(SCB_CCR, IC(1), DC(1));

    arm_dsb();
    arm_isb();
}

static void stm_init_hse(void)
{
    st_writef(RCC_CR, HSEON(1));

    while (!st_readf(RCC_CR, HSERDY));
}

static void stm_init_pll(void)
{
    /* TODO - this should be determined by the target in some way. */

    /* Select HSE/4 input for PLL1 (6 MHz) */
    st_writef(RCC_PLLCKSELR,
              PLLSRC_V(HSE),
              DIVM1(4),
              DIVM2(0),
              DIVM3(0));

    /* Enable PLL1P and PLL1Q */
    st_writef(RCC_PLLCFGR,
              DIVP1EN(1),
              DIVQ1EN(1),
              DIVR1EN(0),
              DIVP2EN(0),
              DIVQ2EN(0),
              DIVR2EN(0),
              DIVP3EN(0),
              DIVQ3EN(0),
              DIVR3EN(0),
              PLL1RGE_V(4_8MHz),
              PLL1VCOSEL_V(WIDE));

    st_writef(RCC_PLL1DIVR,
              DIVN(80 - 1), /* 6 * 80 = 480 MHz  */
              DIVP(1 - 1),  /* 480 / 1 = 480 MHz */
              DIVQ(8 - 1),  /* 480 / 8 = 60 MHz  */
              DIVR(1 - 1));

    st_writef(RCC_CR, PLL1ON(1));
    while (!st_readf(RCC_CR, PLL1RDY));
}

static void stm_init_vos(void)
{
    st_writef(PWR_D3CR, VOS_V(VOS1));
    while (!st_readf(PWR_D3CR, VOSRDY));

    if (STM32H743_USE_VOS0)
    {
        st_writef(RCC_APB4ENR, SYSCFGEN(1));

        /* Set ODEN bit to enter VOS0 */
        st_writef(SYSCFG_PWRCFG, ODEN(1));
        while (!st_readf(PWR_D3CR, VOSRDY));

        st_writef(RCC_APB4ENR, SYSCFGEN(0));
    }
}

static void stm_init_system_clock(void)
{
    /* Enable HCLK /2 divider (CPU is at 480 MHz, HCLK limit is 240 MHz) */
    st_writef(RCC_D1CFGR, HPRE(8));
    while (st_readf(RCC_D1CFGR, HPRE) != 8);

    /* Enable ABP /2 dividers (HCLK/2, APB limit is 120 MHz) */
    st_writef(RCC_D1CFGR, D1PPRE(4));
    st_writef(RCC_D2CFGR, D2PPRE1(4), D2PPRE2(4));
    st_writef(RCC_D3CFGR, D3PPRE(4));

    /* Switch to PLL1P system clock source */
    st_writef(RCC_CFGR, SW_V(PLL1P));
    while (st_readf(RCC_CFGR, SWS) != BV_RCC_CFGR_SW__PLL1P);

    /* Reduce flash access latency */
    st_writef(FLASH_ACR, LATENCY(4), WRHIGHFREQ(2));
    while (st_readf(FLASH_ACR, LATENCY) != 4);
}

static void stm_init_lse(void)
{
    /*
     * Skip if LSE and RTC are already enabled.
     */
    if (st_readf(RCC_BDCR, LSERDY) &&
        st_readf(RCC_BDCR, RTCEN))
        return;

    /*
     * Enable LSE and set it as RTC clock source,
     * then re-enable backup domain write protection.
     */

    st_writef(PWR_CR1, DBP(1));

    /* Reset backup domain */
    st_writef(RCC_BDCR, BDRST(1));
    st_writef(RCC_BDCR, BDRST(0));

    st_writef(RCC_BDCR, LSEON(1), LSEDRV(3));
    while (!st_readf(RCC_BDCR, LSERDY));

    st_writef(RCC_BDCR, RTCEN(1), RTCSEL_V(LSE));
    st_writef(PWR_CR1, DBP(0));
}

void system_init(void)
{
    /* Ensure IRQs are disabled and set vector table address */
    disable_irq();
    cm_write(SCB_VTOR, (uint32_t)__vectors_arm);

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

void tick_start(unsigned int interval_in_ms)
{
    (void)interval_in_ms;

    cm_writef(SYSTICK_CSR, TICKINT(1));
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
    uint32_t start = cm_readf(SYSTICK_CVR, VALUE);
    uint32_t max = cm_readf(SYSTICK_RVR, VALUE);
    uint32_t delay = us * SYSTICK_PER_US;

    for (;;)
    {
        uint32_t value = cm_readf(SYSTICK_CVR, VALUE);
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
