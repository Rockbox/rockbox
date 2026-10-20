/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#include "stddef.h"
#include "target.h"
#include "system.h"
#include "logf.h"
#include "x1000.h"
#include "device.h"

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_JZ,
    "X1000"
};

static struct hwstub_jz_desc_t jz_descriptor =
{
    sizeof(struct hwstub_jz_desc_t),
    HWSTUB_DT_JZ,
    0x1000,
    0
};

void target_udelay(int us)
{
    /* only the low word: delays are far shorter than its period (~12 min) */
    uint32_t start = REG_OST_2CNTL;
    uint32_t count = (uint32_t)us * OST_TICKS_PER_US + 1;
    while(REG_OST_2CNTL - start < count) {}
}

void target_mdelay(int ms)
{
    return target_udelay(ms * 1000);
}

static void ost_init(void)
{
    /* same setup as Rockbox's: OST2 counts EXCLK/4 */
    REG_CPM_CLKGR &= ~CPM_CLKGR_OST;
    REG_OST_ENABLE_CLR = OST_OST2;
    REG_OST_CTRL = (REG_OST_CTRL & ~OST_CTRL_PRESCALE2_MASK) | OST_CTRL_PRESCALE2_BY_4;
    REG_OST_CLEAR = OST_OST2;
    REG_OST_2CNTH = 0;
    REG_OST_2CNTL = 0;
    REG_OST_ENABLE_SET = OST_OST2;
}

static void ccr_set_div(uint32_t div)
{
    REG_CPM_CCR = (REG_CPM_CCR & ~CPM_CCR_DIV_MASK) | div |
                  CPM_CCR_CE_CPU | CPM_CCR_CE_AHB0 | CPM_CCR_CE_AHB2;
    while(REG_CPM_CSR & CPM_CSR_DIV_BUSY_MASK) {}
    REG_CPM_CCR &= ~CPM_CCR_CE_MASK;
}

static void ccr_set_mux(uint32_t mux)
{
    REG_CPM_CCR = (REG_CPM_CCR & ~CPM_CCR_MUX_MASK) | mux;
    while((REG_CPM_CSR & CPM_CSR_MUX_MASK) != CPM_CSR_MUX_MASK) {}
}

/* Same as clk_init() in Rockbox: the SPL only sets up the MPLL (600 MHz),
 * the rest is as the boot ROM left it. */
static void clk_init(void)
{
    /* run from EXCLK while the APLL is reprogrammed */
    ccr_set_mux(CPM_CCR_SEL_SRC_EXCLK | CPM_CCR_SEL_CPLL_SCLK_A |
                CPM_CCR_SEL_H0PLL_SCLK_A | CPM_CCR_SEL_H2PLL_SCLK_A);
    ccr_set_div(CPM_CCR_CDIV(1) | CPM_CCR_L2DIV(1) | CPM_CCR_H0DIV(1) |
                CPM_CCR_H2DIV(1) | CPM_CCR_PDIV(1));

    REG_CPM_APCR &= ~CPM_APCR_ENABLE;
    while(REG_CPM_APCR & CPM_APCR_ON) {}
    /* 24 MHz * 42 = 1008 MHz */
    REG_CPM_APCR = CPM_APCR_BS | CPM_APCR_PLLM(42 - 1) | CPM_APCR_ENABLE;
    while(!(REG_CPM_APCR & CPM_APCR_ON)) {}

    /* CPU 1008 MHz, L2 504 MHz, AHB0/AHB2 200 MHz, PCLK 100 MHz */
    ccr_set_div(CPM_CCR_CDIV(1) | CPM_CCR_L2DIV(2) | CPM_CCR_H0DIV(3) |
                CPM_CCR_H2DIV(3) | CPM_CCR_PDIV(6));
    ccr_set_mux(CPM_CCR_SEL_SRC_APLL | CPM_CCR_SEL_CPLL_SCLK_A |
                CPM_CCR_SEL_H0PLL_MPLL | CPM_CCR_SEL_H2PLL_MPLL);
}

/* Switch the backlight on: it shows that the stub runs, and going off that
 * the player really powered down. The pin is a GPIO output, driven high. */
static void backlight_init(void)
{
#ifdef BACKLIGHT_PC
    uint32_t pin = 1 << BACKLIGHT_PC;
    REG_GPIO_INT_CLR(GPIO_PORT_C) = pin;
    REG_GPIO_MSK_SET(GPIO_PORT_C) = pin;
    REG_GPIO_PAT1_CLR(GPIO_PORT_C) = pin;
    REG_GPIO_PAT0_SET(GPIO_PORT_C) = pin;
#endif
}

void target_init(void)
{
    backlight_init();
    clk_init();
    ost_init();
}

void target_get_desc(int desc, void **buffer)
{
    if(desc == HWSTUB_DT_JZ)
        *buffer = &jz_descriptor;
    else
        *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
    (void) buffer;
    (void) size;
}
