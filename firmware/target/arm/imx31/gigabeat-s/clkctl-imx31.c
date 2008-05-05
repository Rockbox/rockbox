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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "cpu.h"
#include "clkctl-imx31.h"

void imx31_clkctl_module_clock_gating(enum IMX31_CG_LIST cg,
                                      enum IMX31_CG_MODES mode)
{
    volatile unsigned long *reg;
    unsigned long mask;
    int shift;
    int oldlevel;

    if (cg >= CG_NUM_CLOCKS)
        return;

    reg = &CLKCTL_CGR0 + cg / 16;   /* Select CGR0, CGR1, CGR2 */
    shift = 2*(cg % 16);            /* Get field shift */
    mask = CG_MASK << shift;        /* Select field */

    oldlevel = disable_interrupt_save(IRQ_FIQ_STATUS);

    *reg = (*reg & ~mask) | ((mode << shift) & mask);

    restore_interrupt(oldlevel);
}

/* Get the PLL reference clock frequency in HZ */
unsigned int imx31_clkctl_get_pll_ref_clk(void)
{
    if ((CLKCTL_CCMR & (3 << 1)) == (1 << 1))
        return CONFIG_CLK32_FREQ * 1024;
    else
        return CONFIG_HCLK_FREQ;
}

/* Return PLL frequency in HZ */
unsigned int imx31_clkctl_get_pll(enum IMX31_PLLS pll)
{
    uint32_t infreq = imx31_clkctl_get_pll_ref_clk();
    uint32_t reg = (&CLKCTL_MPCTL)[pll];
    uint32_t mfn = reg & 0x3ff;
    uint32_t pd  = ((reg >> 26) & 0xf) + 1;
    uint64_t mfd = ((reg >> 16) & 0x3ff) + 1;
    uint32_t mfi = (reg >> 10) & 0xf;

    mfi = mfi <= 5 ? 5 : mfi;

    return 2*infreq*(mfi * mfd + mfn) / (mfd * pd);
}

unsigned int imx31_clkctl_get_ipg_clk(void)
{
    unsigned int pll = imx31_clkctl_get_pll((CLKCTL_PMCR0 & 0xC0000000) == 0 ?
                               PLL_SERIAL : PLL_MCU);
    uint32_t reg = CLKCTL_PDR0;
    unsigned int max_pdf = ((reg >> 3) & 0x7) + 1;
    unsigned int ipg_pdf = ((reg >> 6) & 0x3) + 1;

    return pll / (max_pdf * ipg_pdf);
}

unsigned int imx31_clkctl_get_ata_clk(void)
{
    return imx31_clkctl_get_ipg_clk();
}
