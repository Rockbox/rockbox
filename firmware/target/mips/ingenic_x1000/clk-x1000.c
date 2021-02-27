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
#include "x1000/cpm.h"
#include "x1000/msc.h"
#include "x1000/aic.h"

static uint32_t pll_get(uint32_t pllreg, uint32_t onbit)
{
    if((pllreg & (1 << onbit)) == 0)
        return 0;

    /* Both PLL registers share the same layout of N/M/OD bits.
     * The max multiplier is 128 and max EXCLK is 26 MHz, so the
     * multiplication should fit within 32 bits without overflow.
     */
    uint32_t rate = X1000_EXCLK_FREQ;
    rate *= jz_vreadf(pllreg, CPM_APCR, PLLM) + 1;
    rate /= jz_vreadf(pllreg, CPM_APCR, PLLN) + 1;
    rate >>= jz_vreadf(pllreg, CPM_APCR, PLLOD);
    return rate;
}

static uint32_t sclk_a_get(void)
{
    switch(jz_readf(CPM_CCR, SEL_SRC)) {
    case 1:  return X1000_EXCLK_FREQ;
    case 2:  return clk_get(X1000_CLK_APLL);
    default: return 0;
    }
}

static uint32_t ccr_get(uint32_t selbit, uint32_t divbit)
{
    uint32_t reg = REG_CPM_CCR;
    uint32_t sel = (reg >> selbit) & 0x3;
    uint32_t div = (reg >> divbit) & 0xf;

    switch(sel) {
    case 1:  return clk_get(X1000_CLK_SCLK_A) / (div + 1);
    case 2:  return clk_get(X1000_CLK_MPLL) / (div + 1);
    default: return 0;
    }
}

static uint32_t ddr_get(void)
{
    uint32_t reg = REG_CPM_DDRCDR;
    uint32_t div = jz_vreadf(reg, CPM_DDRCDR, CLKDIV);

    switch(jz_vreadf(reg, CPM_DDRCDR, CLKSRC)) {
    case 1:  return clk_get(X1000_CLK_SCLK_A) / (div + 1);
    case 2:  return clk_get(X1000_CLK_MPLL) / (div + 1);
    default: return 0;
    }
}

static uint32_t lcd_get(void)
{
    if(jz_readf(CPM_CLKGR, LCD))
        return 0;

    uint32_t reg = REG_CPM_LPCDR;
    uint32_t rate;
    switch(jz_vreadf(reg, CPM_LPCDR, CLKSRC)) {
    case 0: rate = clk_get(X1000_CLK_SCLK_A); break;
    case 1: rate = clk_get(X1000_CLK_MPLL); break;
    default: return 0;
    }

    rate /= jz_vreadf(reg, CPM_LPCDR, CLKDIV) + 1;
    return rate;
}

static uint32_t msc_get(int msc)
{
    if((msc == 0 && jz_readf(CPM_CLKGR, MSC0)) ||
       (msc == 1 && jz_readf(CPM_CLKGR, MSC1)))
        return 0;

    uint32_t reg = REG_CPM_MSC0CDR;
    uint32_t rate;
    switch(jz_vreadf(reg, CPM_MSC0CDR, CLKSRC)) {
    case 0: rate = clk_get(X1000_CLK_SCLK_A); break;
    case 1: rate = clk_get(X1000_CLK_MPLL); break;
    default: return 0;
    }

    uint32_t div;
    if(msc == 0)
        div = jz_readf(CPM_MSC0CDR, CLKDIV);
    else
        div = jz_readf(CPM_MSC1CDR, CLKDIV);

    rate /= 2 * (div + 1);
    rate >>= REG_MSC_CLKRT(msc);
    return rate;
}

static uint32_t i2s_mclk_get(void)
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

static uint32_t i2s_bclk_get(void)
{
    return i2s_mclk_get() / (REG_AIC_I2SDIV + 1);
}

static uint32_t sfc_get(void)
{
    if(jz_readf(CPM_CLKGR, SFC))
        return 0;

    uint32_t reg = REG_CPM_SSICDR;
    uint32_t rate;
    if(jz_vreadf(reg, CPM_SSICDR, SFC_CS) == 0)
        rate = clk_get(X1000_CLK_SCLK_A);
    else
        rate = clk_get(X1000_CLK_MPLL);

    rate /= jz_vreadf(reg, CPM_SSICDR, CLKDIV) + 1;
    return rate;
}

uint32_t clk_get(x1000_clk_t clk)
{
    switch(clk) {
    case X1000_CLK_EXCLK:    return X1000_EXCLK_FREQ;
    case X1000_CLK_APLL:     return pll_get(REG_CPM_APCR, BP_CPM_APCR_ON);
    case X1000_CLK_MPLL:     return pll_get(REG_CPM_MPCR, BP_CPM_MPCR_ON);
    case X1000_CLK_SCLK_A:   return sclk_a_get();
    case X1000_CLK_CPU:      return ccr_get(BP_CPM_CCR_SEL_CPLL, BP_CPM_CCR_CDIV);
    case X1000_CLK_L2CACHE:  return ccr_get(BP_CPM_CCR_SEL_CPLL, BP_CPM_CCR_L2DIV);
    case X1000_CLK_AHB0:     return ccr_get(BP_CPM_CCR_SEL_H0PLL, BP_CPM_CCR_H0DIV);
    case X1000_CLK_AHB2:     return ccr_get(BP_CPM_CCR_SEL_H2PLL, BP_CPM_CCR_H2DIV);
    case X1000_CLK_PCLK:     return ccr_get(BP_CPM_CCR_SEL_H2PLL, BP_CPM_CCR_PDIV);
    case X1000_CLK_DDR:      return ddr_get();
    case X1000_CLK_LCD:      return lcd_get();
    case X1000_CLK_MSC0:     return msc_get(0);
    case X1000_CLK_MSC1:     return msc_get(1);
    case X1000_CLK_I2S_MCLK: return i2s_mclk_get();
    case X1000_CLK_I2S_BCLK: return i2s_bclk_get();
    case X1000_CLK_SFC:      return sfc_get();
    default:                 return 0;
    }
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
        CASE(I2S_MCLK);
        CASE(I2S_BCLK);
        CASE(SFC);
#undef CASE
    default:
        return "NONE";
    }
}

#define CCR_MUX_BITS jz_orm(CPM_CCR, SEL_SRC, SEL_CPLL, SEL_H0PLL, SEL_H2PLL)
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

void clk_set_ccr_div(int cpu, int l2, int ahb0, int ahb2, int pclk)
{
    /* Set new divider configuration */
    jz_writef(CPM_CCR, CDIV(cpu - 1), L2DIV(l2 - 1),
              H0DIV(ahb0 - 1), H2DIV(ahb2 - 1), PDIV(pclk - 1),
              CE_CPU(1), CE_AHB0(1), CE_AHB2(1));

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
