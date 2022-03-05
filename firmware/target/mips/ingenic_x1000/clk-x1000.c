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

#include "system.h"
#include "clk-x1000.h"
#include "boot-x1000.h"
#include "x1000/cpm.h"
#include "x1000/msc.h"
#include "x1000/aic.h"

struct clk_info {
    uint8_t mux_reg;    /* offset from CPM_BASE for mux register */
    uint8_t mux_shift;  /* shift to get mux */
    uint8_t mux_mask;   /* mask to get mux after shifting */
    uint8_t mux_type;   /* type of mux, maps register bits to clock source */
    uint8_t div_reg;    /* offset from CPM_BASE for divider register */
    uint8_t div_shift;  /* shift to get divider */
    uint8_t div_mask;   /* mask to get divider after shifting */
    uint8_t miscbits;   /* inclk shift, clkgr bit, and fake mux value */
};

/* Ugliness to pack/unpack stuff in clk_info->miscbits */
#define INCLK_SHIFT(n)              ((n) & 1)
#define CLKGR_BIT(n)                (((n) & 0x1f) << 1)
#define FAKEMUX(n)                  (((n) & 3) << 6)
#define GET_INCLK_SHIFT(miscbits)   ((miscbits) & 1)
#define GET_CLKGR_BIT(miscbits)     (((miscbits) >> 1) & 0x1f)
#define GET_FAKEMUX(miscbits)       (((miscbits) >> 6) & 3)

/* Clock sources -- the order here is important! */
#define S_STOP   0
#define S_EXCLK  1
#define S_APLL   2
#define S_MPLL   3
#define S_SCLK_A 4

/* Muxes */
#define MUX_TWOBIT    0
#define MUX_ONEBIT    1
#define MUX_USB       2
#define MUX_PHONY     3
#define MUX_NUM_TYPES 4

/* Ugliness to define muxes */
#define MKSEL(x,i)      (((x) & 0xf) << ((i)*4))
#define GETSEL(x,i)     (((x) >> ((i)*4)) & 0xf)
#define STOP(i)         MKSEL(S_STOP, i)
#define EXCLK(i)        MKSEL(S_EXCLK, i)
#define APLL(i)         MKSEL(S_APLL, i)
#define MPLL(i)         MKSEL(S_MPLL, i)
#define SCLK_A(i)       MKSEL(S_SCLK_A, i)
#define MKMUX(a,b,c,d)  (a(0)|b(1)|c(2)|d(3))

/* Ugliness to shorten the clk_info table */
#define JA(x) (JA_CPM_##x & 0xff)
#define BM(x) ((BM_CPM_##x) >> (BP_CPM_##x))
#define BP(x) (BP_CPM_##x)
#define CG(x) CLKGR_BIT(BP_CPM_CLKGR_##x)
#define M(r,f,t) JA(r), BP(r##_##f), BM(r##_##f), t
#define D(r,f)   JA(r), BP(r##_##f), BM(r##_##f)

static const uint16_t clk_mux[MUX_NUM_TYPES] = {
    /*    00      01      10      11 */
    MKMUX(STOP,   SCLK_A, MPLL,   STOP),    /* MUX_TWOBIT */
    MKMUX(SCLK_A, MPLL,   STOP,   STOP),    /* MUX_ONEBIT */
    MKMUX(EXCLK,  EXCLK,  SCLK_A, MPLL),    /* MUX_USB */
    MKMUX(EXCLK,  APLL,   MPLL,   SCLK_A),  /* MUX_PHONY */
};

/* Keep in order with enum x1000_clk_t */
const struct clk_info clk_info[X1000_NUM_SIMPLE_CLKS] = {
    {0, 0, 0, MUX_PHONY, 0, 0, 0, CG(CPU_BIT)|FAKEMUX(0)},
    {0, 0, 0, MUX_PHONY, 0, 0, 0, CG(CPU_BIT)|FAKEMUX(1)},
    {0, 0, 0, MUX_PHONY, 0, 0, 0, CG(CPU_BIT)|FAKEMUX(2)},
    {0, 0, 0, MUX_PHONY, 0, 0, 0, CG(CPU_BIT)|FAKEMUX(3)},
    {M(CCR,     SEL_CPLL,   MUX_TWOBIT), D(CCR,     CDIV),   CG(CPU_BIT)},
    {M(CCR,     SEL_CPLL,   MUX_TWOBIT), D(CCR,     L2DIV),  CG(CPU_BIT)},
    {M(CCR,     SEL_H0PLL,  MUX_TWOBIT), D(CCR,     H0DIV),  CG(AHB0)},
    {M(CCR,     SEL_H2PLL,  MUX_TWOBIT), D(CCR,     H2DIV),  CG(APB0)},
    {M(CCR,     SEL_H2PLL,  MUX_TWOBIT), D(CCR,     PDIV),   CG(APB0)},
    {M(DDRCDR,  CLKSRC,     MUX_TWOBIT), D(DDRCDR,  CLKDIV), CG(DDR)},
    {M(LPCDR,   CLKSRC,     MUX_ONEBIT), D(LPCDR,   CLKDIV), CG(LCD)},
    {M(MSC0CDR, CLKSRC,     MUX_ONEBIT), D(MSC0CDR, CLKDIV), CG(MSC0)|INCLK_SHIFT(1)},
    {M(MSC0CDR, CLKSRC,     MUX_ONEBIT), D(MSC1CDR, CLKDIV), CG(MSC1)|INCLK_SHIFT(1)},
    {M(SSICDR,  SFC_CS,     MUX_ONEBIT), D(SSICDR,  CLKDIV), CG(SFC)},
    {M(USBCDR,  CLKSRC,     MUX_USB),    D(USBCDR,  CLKDIV), CG(OTG)},
};

static uint32_t clk_get_in_rate(uint8_t mux_type, uint32_t mux)
{
    uint32_t reg, onbit;
    uint32_t src = GETSEL(clk_mux[mux_type], mux);
  again:
    switch(src) {
    default:       return 0;
    case S_EXCLK:  return X1000_EXCLK_FREQ;
    case S_APLL:   reg = REG_CPM_APCR; onbit = BM_CPM_APCR_ON; break;
    case S_MPLL:   reg = REG_CPM_MPCR; onbit = BM_CPM_MPCR_ON; break;
    case S_SCLK_A: src = jz_readf(CPM_CCR, SEL_SRC); goto again;
    }

    if(!(reg & onbit))
        return 0;

    uint32_t rate = X1000_EXCLK_FREQ;
    rate *= jz_vreadf(reg, CPM_APCR, PLLM) + 1;
    rate /= jz_vreadf(reg, CPM_APCR, PLLN) + 1;
    rate >>= jz_vreadf(reg, CPM_APCR, PLLOD);
    return rate;
}

static uint32_t clk_get_simple(const struct clk_info* info)
{
    if(REG_CPM_CLKGR & (1 << GET_CLKGR_BIT(info->miscbits)))
        return 0;

    uint32_t base = 0xb0000000;
    uint32_t mux_reg = *(const volatile uint32_t*)(base + info->mux_reg);
    uint32_t div_reg = *(const volatile uint32_t*)(base + info->div_reg);

    uint32_t mux = (mux_reg >> info->mux_shift) & info->mux_mask;
    uint32_t div = (div_reg >> info->div_shift) & info->div_mask;

    mux |= GET_FAKEMUX(info->miscbits);

    uint32_t rate = clk_get_in_rate(info->mux_type, mux);
    rate >>= GET_INCLK_SHIFT(info->miscbits);
    rate /= (div + 1);
    return rate;
}

#ifndef BOOTLOADER
static uint32_t clk_get_i2s_mclk(void)
{
    if(jz_readf(CPM_CLKGR, AIC))
        return 0;

    uint32_t reg = REG_CPM_I2SCDR;
    unsigned long long rate;
    if(jz_vreadf(reg, CPM_I2SCDR, CS) == 0)
        rate = X1000_EXCLK_FREQ;
    else {
        if(jz_vreadf(reg, CPM_I2SCDR, PCS) == 0)
            rate = clk_get(X1000_CLK_SCLK_A);
        else
            rate = clk_get(X1000_CLK_MPLL);

        rate *= jz_vreadf(reg, CPM_I2SCDR, DIV_M);
        rate /= jz_vreadf(reg, CPM_I2SCDR, DIV_N);
    }

    /* Clamp invalid setting to 32 bits */
    if(rate > 0xffffffffull)
        rate = 0xffffffff;

    return rate;
}

static uint32_t clk_get_i2s_bclk(void)
{
    return clk_get_i2s_mclk() / (REG_AIC_I2SDIV + 1);
}

static uint32_t clk_get_decimal(x1000_clk_t clk)
{
    switch(clk) {
    case X1000_CLK_I2S_MCLK: return clk_get_i2s_mclk();
    case X1000_CLK_I2S_BCLK: return clk_get_i2s_bclk();
    default:                 return 0;
    }
}
#endif

uint32_t clk_get(x1000_clk_t clk)
{
#ifndef BOOTLOADER
    if(clk >= X1000_NUM_SIMPLE_CLKS)
        return clk_get_decimal(clk);
#endif

    return clk_get_simple(&clk_info[clk]);
}

const char* clk_get_name(x1000_clk_t clk)
{
    switch(clk) {
#define CASE(x) case X1000_CLK_##x: return #x
        CASE(EXCLK);
        CASE(APLL);
        CASE(MPLL);
        CASE(SCLK_A);
        CASE(CPU);
        CASE(L2CACHE);
        CASE(AHB0);
        CASE(AHB2);
        CASE(PCLK);
        CASE(DDR);
        CASE(LCD);
        CASE(MSC0);
        CASE(MSC1);
        CASE(SFC);
        CASE(USB);
        CASE(I2S_MCLK);
        CASE(I2S_BCLK);
#undef CASE
    default:
        return "NONE";
    }
}

/* At present we've only got 24 MHz targets, and they are all using
 * the same "standard" configuration. */
#if X1000_EXCLK_FREQ == 24000000
void clk_init_early(void)
{
    jz_writef(CPM_MPCR, ENABLE(0));
    while(jz_readf(CPM_MPCR, ON));

    /* 24 MHz * 25 = 600 MHz */
    jz_writef(CPM_MPCR, BS(1), PLLM(25 - 1), PLLN(0), PLLOD(0), ENABLE(1));
    while(jz_readf(CPM_MPCR, ON) == 0);

    /* 600 MHz / 3 = 200 MHz */
    clk_set_ddr(X1000_CLK_MPLL, 3);
}

void clk_init(void)
{
    /* make sure we only initialize the clocks once */
    if(get_boot_flag(BOOT_FLAG_CLK_INIT))
        return;

    clk_set_ccr_mux(CLKMUX_SCLK_A(EXCLK) |
                    CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(SCLK_A) |
                    CLKMUX_AHB2(SCLK_A));
    clk_set_ccr_div(CLKDIV_CPU(1) |
                    CLKDIV_L2(1) |
                    CLKDIV_AHB0(1) |
                    CLKDIV_AHB2(1) |
                    CLKDIV_PCLK(1));

    jz_writef(CPM_APCR, ENABLE(0));
    while(jz_readf(CPM_APCR, ON));

    /* 24 MHz * 42 = 1008 MHz */
    jz_writef(CPM_APCR, BS(1), PLLM(42 - 1), PLLN(0), PLLOD(0), ENABLE(1));
    while(jz_readf(CPM_APCR, ON) == 0);

#if defined(FIIO_M3K) || defined(EROS_QN)
    /* TODO: Allow targets to define their clock frequencies in their config,
     * instead of having this be a random special case. */
    clk_set_ccr_div(CLKDIV_CPU(1) |     /* 1008 MHz */
                    CLKDIV_L2(2) |      /* 504 MHz */
                    CLKDIV_AHB0(5) |    /* 201.6 MHz */
                    CLKDIV_AHB2(5) |    /* 201.6 MHz */
                    CLKDIV_PCLK(10));   /* 100.8 MHz */
    clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) |
                    CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(SCLK_A) |
                    CLKMUX_AHB2(SCLK_A));

    /* DDR to 201.6 MHz */
    clk_set_ddr(X1000_CLK_SCLK_A, 5);

    /* Disable MPLL */
    jz_writef(CPM_MPCR, ENABLE(0));
    while(jz_readf(CPM_MPCR, ON));
#else
    /* Default configuration matching the Ingenic OF */
    clk_set_ccr_div(CLKDIV_CPU(1) |     /* 1008 MHz */
                    CLKDIV_L2(2) |      /* 504 MHz */
                    CLKDIV_AHB0(3) |    /* 200 MHz */
                    CLKDIV_AHB2(3) |    /* 200 MHz */
                    CLKDIV_PCLK(6));    /* 100 MHz */
    clk_set_ccr_mux(CLKMUX_SCLK_A(APLL) |
                    CLKMUX_CPU(SCLK_A) |
                    CLKMUX_AHB0(MPLL) |
                    CLKMUX_AHB2(MPLL));
#endif

    /* mark that clocks have been initialized */
    set_boot_flag(BOOT_FLAG_CLK_INIT);
}
#else
# error "please write a new clk_init() for this EXCLK frequency"
#endif

#define CCR_MUX_BITS jz_orm(CPM_CCR, SEL_SRC, SEL_CPLL, SEL_H0PLL, SEL_H2PLL)
#define CCR_DIV_BITS jz_orm(CPM_CCR, CDIV, L2DIV, H0DIV, H2DIV, PDIV)
#define CSR_MUX_BITS jz_orm(CPM_CSR, SRC_MUX, CPU_MUX, AHB0_MUX, AHB2_MUX)
#define CSR_DIV_BITS jz_orm(CPM_CSR, H2DIV_BUSY, H0DIV_BUSY, CDIV_BUSY)

void clk_set_ccr_mux(uint32_t muxbits)
{
    /* Set new mux configuration */
    uint32_t reg = REG_CPM_CCR;
    reg &= ~CCR_MUX_BITS;
    reg |= muxbits & CCR_MUX_BITS;
    REG_CPM_CCR = reg;

    /* Wait for mux change to complete */
    while((REG_CPM_CSR & CSR_MUX_BITS) != CSR_MUX_BITS);
}

void clk_set_ccr_div(uint32_t divbits)
{
    /* Set new divider configuration */
    uint32_t reg = REG_CPM_CCR;
    reg &= ~CCR_DIV_BITS;
    reg |= divbits & CCR_DIV_BITS;
    reg |= jz_orm(CPM_CCR, CE_CPU, CE_AHB0, CE_AHB2);
    REG_CPM_CCR = reg;

    /* Wait until divider change completes */
    while(REG_CPM_CSR & CSR_DIV_BITS);

    /* Disable CE bits after change */
    jz_writef(CPM_CCR, CE_CPU(0), CE_AHB0(0), CE_AHB2(0));
}

void clk_set_ddr(x1000_clk_t src, uint32_t div)
{
    /* Write new configuration */
    jz_writef(CPM_DDRCDR, CE(1), CLKDIV(div - 1),
              CLKSRC(src == X1000_CLK_MPLL ? 2 : 1));

    /* Wait until mux and divider change are complete */
    while(jz_readf(CPM_CSR, DDR_MUX) == 0);
    while(jz_readf(CPM_DDRCDR, BUSY));

    /* Disable CE bit after change */
    jz_writef(CPM_DDRCDR, CE(0));
}
