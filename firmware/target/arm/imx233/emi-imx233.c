/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "emi-imx233.h"
#include "clkctrl-imx233.h"

struct emi_reg_t
{
    int index;
    uint32_t value;
};

/* hardcode all the register values for the different settings. This is ugly
 * but I don't understand what they mean and it's faster this way so...
 * Recall that everything should be put in iram !
 * Make sure the last value is written to register 40. */

/* Values extracted from Sigmatel linux port (GPL) */

/** mDDR value */
static struct emi_reg_t settings_24M[15] ICONST_ATTR =
{
    {4, 0x01000101}, {7, 0x01000101}, {12, 0x02010002}, {13, 0x06060a02},
    {15, 0x01030000}, {17, 0x2d000102}, {18, 0x20200000}, {19, 0x027f1414},
    {20, 0x01021608}, {21, 0x00000002}, {26, 0x000000b3}, {32, 0x00030687},
    {33, 0x00000003}, {34, 0x000012c1}, {40, 0x00010000}
};

static struct emi_reg_t settings_48M[15] ICONST_ATTR =
{
    {4, 0x01000101}, {7, 0x01000101}, {13, 0x06060a02}, {12, 0x02010002},
    {15, 0x02040000}, {17, 0x2d000104}, {18, 0x1f1f0000}, {19, 0x027f0a0a},
    {20, 0x01021608}, {21, 0x00000004}, {26, 0x0000016f}, {32, 0x00060d17},
    {33, 0x00000006}, {34, 0x00002582}, {40, 0x00020000}
};

static struct emi_reg_t settings_60M[15] ICONST_ATTR =
{
    {4, 0x01000101}, {7, 0x01000101}, {12, 0x02020002}, {13, 0x06060a02},
    {15, 0x02040000}, {17, 0x2d000005}, {18, 0x1f1f0000}, {19, 0x027f0a0a},
    {20, 0x02040a10}, {21, 0x00000006}, {26, 0x000001cc}, {32, 0x00081060},
    {33, 0x00000008}, {34, 0x00002ee5}, {40, 0x00020000}
};

static struct emi_reg_t settings_80M[15] ICONST_ATTR __attribute__((alias("settings_60M")));

static struct emi_reg_t settings_96M[15] ICONST_ATTR =
{
    {4, 0x00000101}, {7, 0x01000001}, {12, 0x02020002}, {13, 0x06070a02},
    {15, 0x03050000}, {17, 0x2d000808}, {18, 0x1f1f0000}, {19, 0x020c1010},
    {20, 0x0305101c}, {21, 0x00000007}, {26, 0x000002e6}, {32, 0x000c1a3b},
    {33, 0x0000000c}, {34, 0x00004b0d}, {40, 0x00030000}
};

static struct emi_reg_t settings_120M[15] ICONST_ATTR =
{
    {4, 0x00000101}, {7, 0x01000001}, {12, 0x02020002}, {13, 0x06070a02},
    {15, 0x03050000}, {17, 0x2300080a}, {18, 0x1f1f0000}, {19, 0x020c1010},
    {20, 0x0306101c}, {21, 0x00000009}, {26, 0x000003a1}, {32, 0x000f20ca},
    {33, 0x0000000f}, {34, 0x00005dca}, {40, 0x00040000}
};

static struct emi_reg_t settings_133M[15] ICONST_ATTR =
{
    {4, 0x00000101}, {7, 0x01000001}, {12, 0x02020002}, {13, 0x06070a02},
    {15, 0x03050000}, {17, 0x2000080a}, {18, 0x1f1f0000}, {19, 0x020c1010},
    {20, 0x0306101c}, {21, 0x0000000a}, {26, 0x00000408}, {32, 0x0010245f},
    {33, 0x00000010}, {34, 0x00006808}, {40, 0x00040000}
};

static struct emi_reg_t settings_155M[15] ICONST_ATTR __attribute__((alias("settings_133M")));

static void set_frequency(unsigned long freq) ICODE_ATTR;

static void set_frequency(unsigned long freq)
{
    /** WARNING all restriction of imx233_emi_set_frequency apply here !! */
    /* Set divider and clear clkgate. */
    unsigned fracdiv;
    unsigned div;
    switch(freq)
    {
        case IMX233_EMIFREQ_151_MHz:
            /* clk_emi@ref_emi/3*18/19 */
            fracdiv = 19;
            div = 3;
            /* ref_emi@480 MHz
             * clk_emi@151.58 MHz */
            break;
        case IMX233_EMIFREQ_130_MHz:
            /* clk_emi@ref_emi/2*18/33 */
            fracdiv = 33;
            div = 2;
            /* ref_emi@480 MHz
             * clk_emi@130.91 MHz */
            break;
        case IMX233_EMIFREQ_64_MHz:
        default:
            /* clk_emi@ref_emi/5*18/27 */
            fracdiv = 27;
            div = 5;
            /* ref_emi@480 MHz
             * clk_emi@64 MHz */
            break;
    }
    BF_WR(CLKCTRL_FRAC, EMIFRAC, fracdiv);
    BF_WR(CLKCTRL_EMI, DIV_EMI, div);
}

void imx233_emi_set_frequency(unsigned long freq) ICODE_ATTR;

void imx233_emi_set_frequency(unsigned long freq)
{
    /** FIXME we rely on the compiler to NOT use the stack here because it's
     * not in iram ! If it's not smart enough, one can switch the switch to use
     * the irq stack since we are running interrupts disable here ! */
    /** BUG for freq<=24 MHz we must keep bypass mode since we run on xtal
     * since this setting is unused by our code so ignore this bug for now */
    /** WARNING DANGER
     * Changing the EMI frequency is complicated because it requires to
     * completely shutdown the external memory interface. We must make sure
     * that this code and all the data it uses in in iram and that no access to
     * the sdram will be made during the change. Care must be taken w.r.t to
     * the cache also. */
    /** FIXME assume that auto-slow is disabled here since that could put some
     * clock below the minimum value and we want to spend as least time as
     * possible in this state anyway.
     * WARNING DANGER don't call any external function when sdram is disabled
     * otherwise you'll poke sdram and trigger a fatal data abort ! */

    /* first disable all interrupts */
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);
    /* flush the cache */
    commit_discard_idcache();
    /* put DRAM into self-refresh mode */
    HW_DRAM_CTL08 |= BM_DRAM_CTL08_SREFRESH;
    /* wait for DRAM to be halted */
    while(!BF_RD(EMI_STAT, DRAM_HALTED));
    /* load timings */
    struct emi_reg_t *regs;
    if(freq <= 24000) regs = settings_24M;
    else if(freq <= 48000) regs = settings_48M;
    else if(freq <= 60000) regs = settings_60M;
    else if(freq <= 80000) regs = settings_80M;
    else if(freq <= 96000) regs = settings_96M;
    else if(freq <= 120000) regs = settings_120M;
    else if(freq <= 133000) regs = settings_133M;
    else regs = settings_155M;

    do
        HW_DRAM_CTLxx(regs->index) = regs->value;
    while((regs++)->index != 40);
    /* switch emi to xtal */
    BF_SET(CLKCTRL_CLKSEQ, BYPASS_EMI);
    /* wait for transition */
    while(BF_RD(CLKCTRL_EMI, BUSY_REF_XTAL));
    /* put emi dll into reset mode */
    HW_EMI_CTRL_SET = BM_EMI_CTRL_DLL_RESET | BM_EMI_CTRL_DLL_SHIFT_RESET;
    /* load the new frequency dividers */
    set_frequency(freq);
    /* switch emi back to pll */
    BF_CLR(CLKCTRL_CLKSEQ, BYPASS_EMI);
    /* wait for transition */
    while(BF_RD(CLKCTRL_EMI, BUSY_REF_EMI));
    /* allow emi dll to lock again */
    HW_EMI_CTRL_CLR = BM_EMI_CTRL_DLL_RESET | BM_EMI_CTRL_DLL_SHIFT_RESET;
    /* wait for lock */
    while(!BF_RD(DRAM_CTL04, DLLLOCKREG));
    /* get DRAM out of self-refresh mode */
    HW_DRAM_CTL08 &= ~BM_DRAM_CTL08_SREFRESH;
    /* wait for DRAM to be to run again */
    while(HW_EMI_STAT & BM_EMI_STAT_DRAM_HALTED);

    restore_interrupt(oldstatus);
}
