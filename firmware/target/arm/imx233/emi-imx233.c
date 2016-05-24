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
#include "string.h"

#include "regs/clkctrl.h"
#include "regs/emi.h"
#include "regs/dram.h"

#define HW_DRAM_CTLxx(xx) (*(&HW_DRAM_CTL00 + (xx)))

struct emi_reg_t
{
    int index;
    uint32_t value;
};

/* hardcode all the register values for the different settings. This avoid
 * computing the register values at runtime since they never change and also
 * avoid wasting some space in iram.
 * Values from IMX233 manual, for Mobile DDR 7.5ns (133 MHz and 64MHz)
 * Make sure the last value is written to register 40. */

static struct emi_reg_t settings_60M[15] ICONST_ATTR =
{
    {4, 0x01000101}, /* DLL bypass mode, concurrent auto-precharge and bank split */
    {7, 0x01000101}, /* Read/write grouping, extra clock for back to back, priority placement */
    {12, 0x02020002}, /* tWR = 2 cycles, tRRD = 1 cycles, tCKE = 2 cycles */
    {13, 0x06060a02}, /* CAS lat gate = 3.0 cycles, CAS lat = 3.0 cycles, tWTR = 2 */
    {15, 0x02040000}, /* tRP = 2 cycles, tDAL = 4 cycles */
    {17, 0x2d000005}, /* DDL: start point = 45, lock = 0, increment = 0, tRC = 5 cycles */
    {18, 0x00000000}, /* */
    {19, 0x01000b0b}, /* DLL: DQS out shift (bypass) = 1, DQS delay bypass (1/0) = 11 / 11 */
    {20, 0x02030a00}, /* tRCD = 2 cycles, tRAS (min) = 3 cycles, DQS write shift (bypass) = 10 */
    {21, 0x00000005}, /* tRFC = 5 cycles */
    {26, 0x000001cc}, /* tREF = 460 cycles */
    {32, 0x00081060}, /* tRAS (max) = 4192 cycles, tXSNR = 8 cycles */
    {33, 0x00000008}, /* tXSR = 8 cycles */
    {34, 0x00002ee5}, /* tINIT = 12005 cycles */
    {40, 0x00020000} /* tPDEX = 2 */
};

static struct emi_reg_t settings_133M[15] ICONST_ATTR =
{
    {4, 0x00000101}, /* concurrent auto-precharge and bank split */
    {7, 0x01000001}, /* Read/write grouping, priority placement */
    {12, 0x02020002}, /* tWR = 2 cycles, tRRD = 2 cycles, tCKE = 2 cycles */
    {13, 0x06070a02}, /* CAS lat gate = 3.0 cycles, CAS lat = 3.5 cycles, tWTR = 2 */
    {15, 0x03050000}, /* tRP = 3 cycles, tDAL = 5 cycles */
    {17, 0x19000f0a}, /* DDL: start point = 25, lock = 0, increment = 15, tRC = 10 cycles */
    {18, 0x1f1f0000}, /* DLL: DQS delay (1/0) = 31 / 31 */
    {19, 0x000a0000}, /* DLL: DQS out shift  = 10 */
    {20, 0x03060023}, /* tRCD = 3 cycles, tRAS (min) = 6 cycles, DQS write shift = 35 */
    {21, 0x0000000a}, /* tRFC = 10 cycles */
    {26, 0x000003f7}, /* tREF = 1015 cycles */
    {32, 0x001023cd}, /* tRAS (max) = 9165 cycles, tXSNR = 16 cycles */
    {33, 0x00000010}, /* tXSR = 16 cycles */
    {34, 0x00006665}, /* tINIT = 26213 cycles */
    {40, 0x00040000} /* tPDEX = 4 */
};

static struct emi_reg_t settings_155M[15] ICONST_ATTR __attribute__((alias("settings_133M")));

static void set_frequency(unsigned long freq) ICODE_ATTR;

#if IMX233_SUBTARGET >= 3700
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
    BF_WR(CLKCTRL_FRAC, CLKGATEEMI(0));
    BF_WR(CLKCTRL_FRAC, EMIFRAC(fracdiv));
    BF_WR(CLKCTRL_EMI, CLKGATE(0));
    BF_WR(CLKCTRL_EMI, DIV_EMI(div));
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

    static unsigned long cur_freq = -1;
    /* avoid changes if unneeded */
    if(cur_freq == freq)
        return;
    cur_freq = freq;

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
    switch(freq)
    {
        case IMX233_EMIFREQ_151_MHz:
            regs = settings_155M;
            break;
        case IMX233_EMIFREQ_130_MHz:
            regs = settings_133M;
            break;
        case IMX233_EMIFREQ_64_MHz:
        default:
            regs = settings_60M;
            break;
    }

    do
        HW_DRAM_CTLxx(regs->index) = regs->value;
    while((regs++)->index != 40);
    /* switch emi to xtal */
    BF_SET(CLKCTRL_CLKSEQ, BYPASS_EMI);
    /* wait for transition */
    while(BF_RD(CLKCTRL_EMI, BUSY_REF_XTAL));
    /* put emi dll into reset mode */
    // FIXME Unsure about what to do for stmp37xx
#if IMX233_SUBTARGET >= 3780
    HW_EMI_CTRL_SET = BM_EMI_CTRL_DLL_RESET | BM_EMI_CTRL_DLL_SHIFT_RESET;
#endif
    /* load the new frequency dividers */
    set_frequency(freq);
    /* switch emi back to pll */
    BF_CLR(CLKCTRL_CLKSEQ, BYPASS_EMI);
    /* wait for transition */
    while(BF_RD(CLKCTRL_EMI, BUSY_REF_EMI));
    /* allow emi dll to lock again */
#if IMX233_SUBTARGET >= 3780
    HW_EMI_CTRL_CLR = BM_EMI_CTRL_DLL_RESET | BM_EMI_CTRL_DLL_SHIFT_RESET;
#endif
    /* wait for lock */
    while(!BF_RD(DRAM_CTL04, DLLLOCKREG));
    /* get DRAM out of self-refresh mode */
    HW_DRAM_CTL08 &= ~BM_DRAM_CTL08_SREFRESH;
    /* wait for DRAM to be to run again */
    while(HW_EMI_STAT & BM_EMI_STAT_DRAM_HALTED);

    restore_interrupt(oldstatus);
}
#endif

struct imx233_emi_info_t imx233_emi_get_info(void)
{
    struct imx233_emi_info_t info;
    memset(&info, 0,  sizeof(info));
#if IMX233_SUBTARGET >= 3700
    info.rows = 13 - BF_RD(DRAM_CTL10, ADDR_PINS);
    info.columns = 12 - BF_RD(DRAM_CTL11, COLUMN_SIZE);
    info.cas = BF_RD(DRAM_CTL13, CASLAT_LIN);
    info.banks = 4;
    info.chips = __builtin_popcount(BF_RD(DRAM_CTL14, CS_MAP));
    info.size = 2 * (1 << (info.rows + info.columns)) * info.chips * info.banks;
#endif
    return info;
}