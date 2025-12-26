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
#include "clock-stm32h7.h"
#include "panic.h"
#include "regs/stm32h743/flash.h"
#include "regs/stm32h743/fmc.h"
#include "regs/stm32h743/pwr.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/syscfg.h"

/* Flag to use VOS0 */
#define STM32H743_USE_VOS0      (CPU_FREQ > 400000000)

static void init_hse(void)
{
    reg_writef(RCC_CR, HSEON(1));

    while (!reg_readf(RCC_CR, HSERDY));
}

static void init_pll(void)
{
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

static void init_vos(void)
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

static void init_system_clock(void)
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

static void init_lse(void)
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

void stm_target_clock_init(void)
{
    init_hse();
    init_pll();
    init_vos();
    init_system_clock();
    init_lse();
}
