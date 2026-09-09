/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __CLK_X1600_H__
#define __CLK_X1600_H__

#include "config.h"
#include "x1600/cpm.h"
#include <stdint.h>

/* Used as arguments to clk_set_ccr_mux() */
#define CLKMUX_SCLK_A(x) jz_orf(CPM_CPCCR, SEL_SRC_V(x))
#define CLKMUX_CPU(x)    jz_orf(CPM_CPCCR, SEL_CPLL_V(x))
#define CLKMUX_AHB0(x)   jz_orf(CPM_CPCCR, SEL_H0PLL_V(x))
#define CLKMUX_AHB2(x)   jz_orf(CPM_CPCCR, SEL_H2PLL_V(x))

/* Arguments to clk_set_ccr_div(); all five dividers are "value + 1" */
#define CLKDIV_CPU(x)    jz_orf(CPM_CPCCR, CDIV((x) - 1))
/* The X1600 calls this field L2CDIV where the X1000 calls it L2DIV */
#define CLKDIV_L2(x)     jz_orf(CPM_CPCCR, L2CDIV((x) - 1))
#define CLKDIV_AHB0(x)   jz_orf(CPM_CPCCR, H0DIV((x) - 1))
#define CLKDIV_AHB2(x)   jz_orf(CPM_CPCCR, H2DIV((x) - 1))
#define CLKDIV_PCLK(x)   jz_orf(CPM_CPCCR, PDIV((x) - 1))

/* EPLL is first-class here and there is no X1600_CLK_USB, unlike the X1000 */
typedef enum x1600_clk_t {
    X1600_CLK_EXCLK,
    X1600_CLK_APLL,
    X1600_CLK_MPLL,
    X1600_CLK_EPLL,
    X1600_CLK_SCLK_A,
    X1600_CLK_CPU,
    X1600_CLK_L2CACHE,
    X1600_CLK_AHB0,
    X1600_CLK_AHB2,
    X1600_CLK_PCLK,
    X1600_CLK_DDR,
    X1600_CLK_LCD,
    X1600_CLK_MSC0,
    X1600_CLK_MSC1,
    X1600_CLK_SFC,
    X1600_CLK_SSI,
    X1600_CLK_PWM,
    X1600_CLK_I2S_MCLK,
    X1600_CLK_I2S_BCLK,
    X1600_CLK_COUNT,
} x1600_clk_t;

/* Calculate the current frequency of a clock; 0 if stopped or gated */
extern uint32_t clk_get(x1600_clk_t clk);

/* Get the name of a clock for debug purposes */
extern const char* clk_get_name(x1600_clk_t clk);

/* Measure the core clock against the OST. Takes about 2 ms and requires a
 * running OST. */
extern uint32_t clk_measure_cpu(void);

/* Clock initialization; the _early form is the SPL's, for PLL and DDR */
extern void clk_init_early(void) INIT_ATTR;
extern void clk_init(void) INIT_ATTR;

/* Sets system clock multiplexers */
extern void clk_set_ccr_mux(uint32_t muxbits) INIT_ATTR;

/* Sets system clock dividers */
extern void clk_set_ccr_div(uint32_t divbits) INIT_ATTR;

/* Sets DDR clock source and divider */
extern void clk_set_ddr(x1600_clk_t src, uint32_t div) INIT_ATTR;

/* Program the I2S MCLK for a sample rate, returning what was programmed or 0.
 * 'mult' is the MCLK/fs ratio; the caller owns AIC_I2SDIV below it. */
extern uint32_t clk_set_i2s_mclk(uint32_t fs, uint32_t mult);

/* Decompose a fractional-N PLL control word, for the debug menu and tests */
extern uint32_t clk_decode_pll(uint32_t pllreg);

/* Returns the smallest n such that infreq/n <= outfreq */
static inline uint32_t clk_calc_div(uint32_t infreq, uint32_t outfreq)
{
    return (infreq + (outfreq - 1)) / outfreq;
}

/* Returns the smallest n such that infreq <= outfreq * 2^n */
static inline uint32_t clk_calc_shift(uint32_t infreq, uint32_t outfreq)
{
    uint32_t div = clk_calc_div(infreq, outfreq);
    return div > 1 ? 32 - __builtin_clz(div - 1) : 0;
}

/* Number of PLL or mux waits that timed out. */
extern volatile uint32_t clk_timeouts;

#endif /* __CLK_X1600_H__ */
