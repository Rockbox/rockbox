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

unsigned int ccm_get_src_pll(void)
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

/* Get the PLL reference clock frequency in HZ */
unsigned int ccm_get_pll_ref_clk(void)
{
    if ((CCM_CCMR & (3 << 1)) == (1 << 1))
        return CONFIG_CKIL_FREQ * 1024;
    else
        return CONFIG_CKIH_FREQ;
}

/* Return PLL frequency in HZ */
unsigned int ccm_get_pll(enum IMX31_PLLS pll)
{
    uint32_t infreq = ccm_get_pll_ref_clk();
    uint32_t reg = (&CCM_MPCTL)[pll];
    uint32_t mfn = reg & 0x3ff;
    uint32_t pd  = ((reg >> 26) & 0xf) + 1;
    uint64_t mfd = ((reg >> 16) & 0x3ff) + 1;
    uint32_t mfi = (reg >> 10) & 0xf;

    mfi = mfi <= 5 ? 5 : mfi;

    return 2*infreq*(mfi * mfd + mfn) / (mfd * pd);
}

unsigned int ccm_get_ipg_clk(void)
{
    unsigned int pllnum = ccm_get_src_pll();
    unsigned int pll = ccm_get_pll(pllnum);
    uint32_t reg = CCM_PDR0;
    unsigned int max_pdf = ((reg >> 3) & 0x7) + 1;
    unsigned int ipg_pdf = ((reg >> 6) & 0x3) + 1;

    return pll / (max_pdf * ipg_pdf);
}

unsigned int ccm_get_ahb_clk(void)
{
    unsigned int pllnum = ccm_get_src_pll();
    unsigned int pll = ccm_get_pll(pllnum);
    unsigned int max_pdf = ((CCM_PDR0 >> 3) & 0x7) + 1;

    return pll / max_pdf;
}

unsigned int ccm_get_ata_clk(void)
{
    return ccm_get_ipg_clk();
}
