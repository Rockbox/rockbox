/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __CLK_X1000_H__
#define __CLK_X1000_H__

#include <stdint.h>
#include "x1000/cpm.h"

/* Used as arguments to clk_set_ccr_mux() */
#define CLKMUX_SCLK_A(x) jz_orf(CPM_CCR, SEL_SRC_V(x))
#define CLKMUX_CPU(x)    jz_orf(CPM_CCR, SEL_CPLL_V(x))
#define CLKMUX_AHB0(x)   jz_orf(CPM_CCR, SEL_H0PLL_V(x))
#define CLKMUX_AHB2(x)   jz_orf(CPM_CCR, SEL_H2PLL_V(x))

/* Arguments to clk_set_ccr_div() */
#define CLKDIV_CPU(x)    jz_orf(CPM_CCR, CDIV((x) - 1))
#define CLKDIV_L2(x)     jz_orf(CPM_CCR, L2DIV((x) - 1))
#define CLKDIV_AHB0(x)   jz_orf(CPM_CCR, H0DIV((x) - 1))
#define CLKDIV_AHB2(x)   jz_orf(CPM_CCR, H2DIV((x) - 1))
#define CLKDIV_PCLK(x)   jz_orf(CPM_CCR, PDIV((x) - 1))

typedef enum x1000_clk_t {
    X1000_CLK_EXCLK,
    X1000_CLK_APLL,
    X1000_CLK_MPLL,
    X1000_CLK_SCLK_A,
    X1000_CLK_CPU,
    X1000_CLK_L2CACHE,
    X1000_CLK_AHB0,
    X1000_CLK_AHB2,
    X1000_CLK_PCLK,
    X1000_CLK_DDR,
    X1000_CLK_LCD,
    X1000_CLK_MSC0,
    X1000_CLK_MSC1,
    X1000_CLK_SFC,
    X1000_CLK_USB,
    X1000_NUM_SIMPLE_CLKS,
    X1000_CLK_I2S_MCLK = X1000_NUM_SIMPLE_CLKS,
    X1000_CLK_I2S_BCLK,
    X1000_CLK_COUNT,
} x1000_clk_t;

/* Calculate the current frequency of a clock */
extern uint32_t clk_get(x1000_clk_t clk);

/* Get the name of a clock for debug purposes */
extern const char* clk_get_name(x1000_clk_t clk);

/* Clock initialization */
extern void clk_init_early(void);
extern void clk_init(void);

/* Sets system clock multiplexers */
extern void clk_set_ccr_mux(uint32_t muxbits);

/* Sets system clock dividers */
extern void clk_set_ccr_div(uint32_t divbits);

/* Sets DDR clock source and divider */
extern void clk_set_ddr(x1000_clk_t src, uint32_t div);

/* Returns the smallest n such that infreq/n <= outfreq */
static inline uint32_t clk_calc_div(uint32_t infreq, uint32_t outfreq)
{
    return (infreq + (outfreq - 1)) / outfreq;
}

/* Returns the smallest n such that (infreq >> n) <= outfreq */
static inline uint32_t clk_calc_shift(uint32_t infreq, uint32_t outfreq)
{
    uint32_t div = clk_calc_div(infreq, outfreq);
    return __builtin_clz(div) ^ 31;
}

#endif /* __CLK_X1000_H__ */
