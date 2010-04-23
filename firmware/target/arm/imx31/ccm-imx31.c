/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 Michael Sevakis
 *
 * Clock control functions for IMX31 processor
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
#include "cpu.h"
#include "ccm-imx31.h"

/* Return the current source pll for MCU */
enum IMX31_PLLS ccm_get_src_pll(void)
{
    return (CCM_PMCR0 & 0xC0000000) == 0 ? PLL_SERIAL : PLL_MCU;
}

void ccm_module_clock_gating(enum IMX31_CG_LIST cg, enum IMX31_CG_MODES mode)
{
    volatile unsigned long *reg;
    unsigned long mask;
    int shift;

    if (cg >= CG_NUM_CLOCKS)
        return;

    reg = &CCM_CGR0 + cg / 16;  /* Select CGR0, CGR1, CGR2 */
    shift = 2*(cg % 16);        /* Get field shift */
    mask = CG_MASK << shift;    /* Select field */

    imx31_regmod32(reg, mode << shift, mask);
}

/* Decode PLL output frequency from register value */
unsigned int ccm_calc_pll_rate(unsigned int infreq, unsigned long regval)
{
    uint32_t mfn = regval & 0x3ff;
    uint32_t pd  = ((regval >> 26) & 0xf) + 1;
    uint32_t mfd = ((regval >> 16) & 0x3ff) + 1;
    uint32_t mfi = (regval >> 10) & 0xf;

    mfi = mfi <= 5 ? 5 : mfi;

    return 2ull*infreq*(mfi * mfd + mfn) / (mfd * pd);
}

/* Get the PLL reference clock frequency in HZ */
unsigned int ccm_get_pll_ref_clk_rate(void)
{
    if ((CCM_CCMR & (3 << 1)) == (1 << 1))
        return CONFIG_CKIL_FREQ * 1024;
    else
        return CONFIG_CKIH_FREQ;
}

/* Return PLL frequency in HZ */
unsigned int ccm_get_pll_rate(enum IMX31_PLLS pll)
{
    return ccm_calc_pll_rate(ccm_get_pll_ref_clk_rate(), (&CCM_MPCTL)[pll]);
}

unsigned int ccm_get_mcu_clk(void)
{
    unsigned int pllnum = ccm_get_src_pll();
    unsigned int fpll = ccm_get_pll_rate(pllnum);
    unsigned int mcu_podf = (CCM_PDR0 & 0x7) + 1;

    return fpll / mcu_podf;
}

unsigned int ccm_get_ipg_clk(void)
{
    unsigned int pllnum = ccm_get_src_pll();
    unsigned int fpll = ccm_get_pll_rate(pllnum);
    uint32_t reg = CCM_PDR0;
    unsigned int max_pdf = ((reg >> 3) & 0x7) + 1;
    unsigned int ipg_pdf = ((reg >> 6) & 0x3) + 1;

    return fpll / (max_pdf * ipg_pdf);
}

unsigned int ccm_get_ahb_clk(void)
{
    unsigned int pllnum = ccm_get_src_pll();
    unsigned int fpll = ccm_get_pll_rate(pllnum);
    unsigned int max_pdf = ((CCM_PDR0 >> 3) & 0x7) + 1;

    return fpll / max_pdf;
}

unsigned int ccm_get_ata_clk(void)
{
    return ccm_get_ipg_clk();
}

/* Write new values to the current PLL and post-dividers */
void ccm_set_mcupll_and_pdr(unsigned long pllctl, unsigned long pdr)
{
    unsigned int pll = ccm_get_src_pll();
    volatile unsigned long *pllreg = &(&CCM_MPCTL)[pll];
    unsigned long fref = ccm_get_pll_ref_clk_rate();
    unsigned long curfreq = ccm_calc_pll_rate(fref, *pllreg);
    unsigned long newfreq = ccm_calc_pll_rate(fref, pllctl);

    if (newfreq > curfreq)
        CCM_PDR0 = pdr;

    *pllreg = pllctl;

    if (newfreq <= curfreq)
        CCM_PDR0 = pdr;
}
