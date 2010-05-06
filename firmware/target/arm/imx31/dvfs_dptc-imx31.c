/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Michael Sevakis
 *
 * i.MX31 DVFS and DPTC drivers
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
#include "config.h"
#include "system.h"
#include "logf.h"
#include "mc13783.h"
#include "iomuxc-imx31.h"
#include "ccm-imx31.h"
#include "avic-imx31.h"
#include "dvfs_dptc-imx31.h"
#include "dvfs_dptc_tables-target.h"

/* Most of the code in here is based upon the Linux BSP provided by Freescale
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved. */

/* The current DVFS index level */
static volatile unsigned int dvfs_level = DVFS_LEVEL_DEFAULT;
/* The current DPTC working point */
static volatile unsigned int dptc_wp = DPTC_WP_DEFAULT;


static void update_dptc_counts(unsigned int level, unsigned int wp)
{
    int oldlevel = disable_irq_save();
    const struct dptc_dcvr_table_entry *entry = &dptc_dcvr_table[level][wp];

    CCM_DCVR0 = entry->dcvr0;
    CCM_DCVR1 = entry->dcvr1;
    CCM_DCVR2 = entry->dcvr2;
    CCM_DCVR3 = entry->dcvr3;

    restore_irq(oldlevel);
}


static uint32_t check_regulator_setting(uint32_t setting)
{
    /* Simply a safety check *in case* table gets scrambled */
    if (setting < VOLTAGE_SETTING_MIN)
        setting = VOLTAGE_SETTING_MIN;
    else if (setting > VOLTAGE_SETTING_MAX)
        setting = VOLTAGE_SETTING_MAX;

    return setting;
}


/** DVFS **/
static bool dvfs_running = false; /* Has driver enabled DVFS? */

/* Request tracking since boot */
unsigned int dvfs_nr_dn = 0;
unsigned int dvfs_nr_up = 0;
unsigned int dvfs_nr_pnc = 0;
unsigned int dvfs_nr_no = 0;

static void dvfs_stop(void);


/* Wait for the UPDTEN flag to be set so that all bits may be written */
static inline void wait_for_dvfs_update_en(void)
{
    while (!(CCM_PMCR0 & CCM_PMCR0_UPDTEN));
}


static void do_dvfs_update(unsigned int level, bool in_isr)
{
    const struct dvfs_clock_table_entry *setting = &dvfs_clock_table[level];
    unsigned long pmcr0 = CCM_PMCR0;

    if (pmcr0 & CCM_PMCR0_DPTEN)
    {
        /* Ignore voltage change request from DPTC. Voltage is *not* valid. */
        pmcr0 &= ~CCM_PMCR0_DPVCR;
    }

    pmcr0 &= ~CCM_PMCR0_VSCNT;

    if (level < ((pmcr0 & CCM_PMCR0_DVSUP) >> CCM_PMCR0_DVSUP_POS))
    { 
        pmcr0 |= CCM_PMCR0_UDSC; /* Up scaling, increase */
        pmcr0 |= setting->vscnt << CCM_PMCR0_VSCNT_POS;
    }
    else
    {
        pmcr0 &= ~CCM_PMCR0_UDSC; /* Down scaling, decrease */
        pmcr0 |= 0x1 << CCM_PMCR0_VSCNT_POS;
    }

    /* DVSUP (new frequency index) setup */
    pmcr0 = (pmcr0 & ~CCM_PMCR0_DVSUP) | (level << CCM_PMCR0_DVSUP_POS);

    dvfs_level = level;

    if ((setting->pll_num << CCM_PMCR0_DFSUP_MCUPLL_POS) ^
        (pmcr0 & CCM_PMCR0_DFSUP_MCUPLL))
    {
        /* Update pll and post-dividers. */
        pmcr0 ^= CCM_PMCR0_DFSUP_MCUPLL;
        pmcr0 &= ~CCM_PMCR0_DFSUP_POST_DIVIDERS;
    }
    else
    {
        /* Post-dividers update only */
        pmcr0 |= CCM_PMCR0_DFSUP_POST_DIVIDERS;
    }

    CCM_PMCR0 = pmcr0;
    /* Note: changes to frequency with ints unmaked seem to cause spurious
     * DVFS interrupts with value CCM_PMCR0_FSVAI_NO_INT. These aren't
     * supposed to happen. Only do the lengthy delay with them enabled iff
     * called from the IRQ handler. */
    if (in_isr)
        enable_irq();
    udelay(100); /* Software wait for voltage ramp-up */
    if (in_isr)
        disable_irq();
    CCM_PDR0 = setting->pdr_val;

    if (!(pmcr0 & CCM_PMCR0_DFSUP_POST_DIVIDERS))
    {
        /* Update the PLL settings */
        if (pmcr0 & CCM_PMCR0_DFSUP_MCUPLL)
            CCM_MPCTL = setting->pll_val;
        else
            CCM_SPCTL = setting->pll_val;
    }

    cpu_frequency = ccm_get_mcu_clk();

    if (pmcr0 & CCM_PMCR0_DPTEN)
    {
        update_dptc_counts(level, dptc_wp);
        /* Enable DPTC to request voltage changes. Voltage is valid. */
        CCM_PMCR0 |= CCM_PMCR0_DPVCR;
        udelay(2);
        CCM_PMCR0 |= CCM_PMCR0_DPVV;
    }
}


/* Start DVFS, change the set point and stop it */
static void set_current_dvfs_level(unsigned int level)
{
    int oldlevel = disable_irq_save();

    CCM_PMCR0 |= CCM_PMCR0_DVFEN;

    wait_for_dvfs_update_en();

    do_dvfs_update(level, false);

    wait_for_dvfs_update_en();

    CCM_PMCR0 &= ~CCM_PMCR0_DVFEN;

    restore_irq(oldlevel);
}


/* DVFS Interrupt handler */
static void __attribute__((used)) dvfs_int(void)
{
    unsigned long pmcr0 = CCM_PMCR0;
    unsigned long fsvai = pmcr0 & CCM_PMCR0_FSVAI;
    unsigned int level = (pmcr0 & CCM_PMCR0_DVSUP) >> CCM_PMCR0_DVSUP_POS;

    if (pmcr0 & CCM_PMCR0_FSVAIM)
        return; /* Do nothing. DVFS interrupt is masked. */

    if (!(pmcr0 & CCM_PMCR0_UPDTEN))
        return; /* Do nothing. DVFS didn't finish previous flow update. */

    switch (fsvai)
    {
    case CCM_PMCR0_FSVAI_DECREASE:
        if (level >= DVFS_NUM_LEVELS - 1)
            return; /* DVFS already at lowest level */

        /* Upon the DECREASE event, the frequency will be changed to the next
         * higher state index. */
        while (((1u << ++level) & DVFS_LEVEL_MASK) == 0);

        dvfs_nr_dn++;
        break;

    /* Single-step frequency increase */
    case CCM_PMCR0_FSVAI_INCREASE:
        if (level == 0)
            return; /* DVFS already at highest level */

        /* Upon the INCREASE event, the frequency will be changed to the next
         * lower state index. */
        while (((1u << --level) & DVFS_LEVEL_MASK) == 0);

        dvfs_nr_up++;
        break;

    /* Right to highest if panic */
    case CCM_PMCR0_FSVAI_INCREASE_NOW:
        if (level == 0)
            return; /* DVFS already at highest level */

        /* Upon the INCREASE_NOW event, the frequency will be increased to
         * the maximum (index 0). */
        level = 0;
        dvfs_nr_pnc++;
        break;

    case CCM_PMCR0_FSVAI_NO_INT:
        dvfs_nr_no++;
        return;     /* Do nothing. Freq change is not required */
    } /* end switch */

    do_dvfs_update(level, true);
}


/* Interrupt vector for DVFS */
static __attribute__((naked, interrupt("IRQ"))) void CCM_DVFS_HANDLER(void)
{
    /* Audio can glitch with the long udelay if nested IRQ isn't allowed. */
    AVIC_NESTED_NI_CALL_PROLOGUE(INT_PRIO_DVFS, 32*4);
    asm volatile ("bl dvfs_int");
    AVIC_NESTED_NI_CALL_EPILOGUE(32*4);
}


/* Initialize the DVFS hardware */
static void dvfs_init(void)
{
    if (CCM_PMCR0 & CCM_PMCR0_DVFEN)
    {
        /* Turn it off first. Really, shouldn't happen though. */
        dvfs_running = true;
        dvfs_stop();
    }

    /* Combine SW1A and SW1B DVS pins for a possible five DVS levels
     * per working point. Four, MAXIMUM, are actually used, one for each
     * frequency. */
    mc13783_set(MC13783_ARBITRATION_SWITCHERS, MC13783_SW1ABDVS);

    /* Set DVS speed to 25mV every 4us. */
    mc13783_write_masked(MC13783_SWITCHERS4, MC13783_SW1ADVSSPEED_4US,
                         MC13783_SW1ADVSSPEED);

    /* Set DVFS pins to functional outputs. Input mode and pad setting is
     * fixed in hardware. */
    iomuxc_set_pin_mux(IOMUXC_DVFS0,
                       IOMUXC_MUX_OUT_FUNCTIONAL | IOMUXC_MUX_IN_NONE);
    iomuxc_set_pin_mux(IOMUXC_DVFS1,
                       IOMUXC_MUX_OUT_FUNCTIONAL | IOMUXC_MUX_IN_NONE);

#ifndef DVFS_NO_PWRRDY
    /* Configure PWRRDY signal pin. */
    imx31_regclr32(&GPIO1_GDIR, (1 << 5));
    iomuxc_set_pin_mux(IOMUXC_GPIO1_5,
                       IOMUXC_MUX_OUT_FUNCTIONAL | IOMUXC_MUX_IN_FUNCTIONAL);
#endif
    
    /* Initialize DVFS signal weights and detection modes. */
    int i;
    for (i = 0; i < 16; i++)
    {
        dvfs_set_lt_weight(i, lt_signals[i].weight);
        dvfs_set_lt_detect(i, lt_signals[i].detect);
    }

    /* Set up LTR0. */
    imx31_regmod32(&CCM_LTR0,
                   DVFS_UPTHR << CCM_LTR0_UPTHR_POS |
                   DVFS_DNTHR << CCM_LTR0_DNTHR_POS |
                   DVFS_DIV3CK << CCM_LTR0_DIV3CK_POS,
                   CCM_LTR0_UPTHR | CCM_LTR0_DNTHR | CCM_LTR0_DIV3CK);

    /* Set up LTR1. */
    imx31_regmod32(&CCM_LTR1,
                   DVFS_DNCNT << CCM_LTR1_DNCNT_POS |
                   DVFS_UPCNT << CCM_LTR1_UPCNT_POS |
                   DVFS_PNCTHR << CCM_LTR1_PNCTHR_POS |
                   CCM_LTR1_LTBRSR,
                   CCM_LTR1_DNCNT | CCM_LTR1_UPCNT |
                   CCM_LTR1_PNCTHR | CCM_LTR1_LTBRSR);

    /* Set up LTR2-- EMA configuration. */
    imx31_regmod32(&CCM_LTR2, DVFS_EMAC << CCM_LTR2_EMAC_POS,
                   CCM_LTR2_EMAC);

    /* DVFS interrupt goes to MCU. Mask load buffer full interrupt. */
    imx31_regset32(&CCM_PMCR0, CCM_PMCR0_DVFIS | CCM_PMCR0_LBMI);

    /* Initialize current core PLL and dividers for default level. Assumes
     * clocking scheme has been set up appropriately in other init code. */
    ccm_set_mcupll_and_pdr(dvfs_clock_table[DVFS_LEVEL_DEFAULT].pll_val,
                           dvfs_clock_table[DVFS_LEVEL_DEFAULT].pdr_val);

    /* Set initial level and working point. */
    set_current_dvfs_level(DVFS_LEVEL_DEFAULT);

    logf("DVFS: Initialized");
}


/* Start the DVFS hardware */
static void dvfs_start(void)
{
    int oldlevel;

    /* Have to wait at least 3 div3 clocks before enabling after being
     * stopped. */
    udelay(1500);

    oldlevel = disable_irq_save();

    if (!dvfs_running)
    {
        dvfs_running = true;

        /* Unmask DVFS interrupt source and enable DVFS. */
        avic_enable_int(INT_CCM_DVFS, INT_TYPE_IRQ, INT_PRIO_DVFS,
                        CCM_DVFS_HANDLER);

        CCM_PMCR0 = (CCM_PMCR0 & ~CCM_PMCR0_FSVAIM) | CCM_PMCR0_DVFEN;
    }

    restore_irq(oldlevel);

    logf("DVFS: started");
}


/* Stop the DVFS hardware and return to default frequency */
static void dvfs_stop(void)
{
    int oldlevel = disable_irq_save();

    if (dvfs_running)
    {
        /* Mask DVFS interrupts. */
        CCM_PMCR0 |= CCM_PMCR0_FSVAIM | CCM_PMCR0_LBMI;
        avic_disable_int(INT_CCM_DVFS);

        if (((CCM_PMCR0 & CCM_PMCR0_DVSUP) >> CCM_PMCR0_DVSUP_POS) !=
                DVFS_LEVEL_DEFAULT)
        {
            /* Set default frequency level */
            wait_for_dvfs_update_en();
            do_dvfs_update(DVFS_LEVEL_DEFAULT, false);
            wait_for_dvfs_update_en();
        }

        /* Disable DVFS. */
        CCM_PMCR0 &= ~CCM_PMCR0_DVFEN;
        dvfs_running = false;
    }

    restore_irq(oldlevel);

    logf("DVFS: stopped");
}


/** DPTC **/

/* Request tracking since boot */
static bool dptc_running = false; /* Has driver enabled DPTC? */

unsigned int dptc_nr_dn = 0;
unsigned int dptc_nr_up = 0;
unsigned int dptc_nr_pnc = 0;
unsigned int dptc_nr_no = 0;

static struct spi_transfer_desc dptc_pmic_xfer; /* Transfer descriptor */
static const unsigned char dptc_pmic_regs[2] =  /* Register subaddresses */
{ MC13783_SWITCHERS0, MC13783_SWITCHERS1 };
static uint32_t dptc_reg_shadows[2];            /* shadow regs */
static uint32_t dptc_regs_buf[2];               /* buffer for async write */


/* Enable DPTC and unmask interrupt. */
static void enable_dptc(void)
{
    int oldlevel = disable_irq_save();

    /* Enable DPTC, assert voltage change request. */
    CCM_PMCR0 = (CCM_PMCR0 & ~CCM_PMCR0_PTVAIM) | CCM_PMCR0_DPTEN |
                    CCM_PMCR0_DPVCR;

    udelay(2);

    /* Set voltage valid *after* setting change request */
    CCM_PMCR0 |= CCM_PMCR0_DPVV;

    restore_irq(oldlevel);
}


/* Called after asynchronous PMIC write is completed */
static void dptc_transfer_done_callback(struct spi_transfer_desc *xfer)
{
    if (xfer->count != 0)
        return;

    update_dptc_counts(dvfs_level, dptc_wp);

    if (dptc_running)
        enable_dptc();
}


/* Handle the DPTC interrupt and sometimes the manual setting */
static void dptc_int(unsigned long pmcr0)
{
    const union dvfs_dptc_voltage_table_entry *entry;
    uint32_t sw1a, sw1advs, sw1bdvs, sw1bstby;
    uint32_t switchers0, switchers1;

    int wp = dptc_wp;

    /* Mask DPTC interrupt and disable DPTC until the change request is
     * serviced. */
    CCM_PMCR0 = (pmcr0 & ~CCM_PMCR0_DPTEN) | CCM_PMCR0_PTVAIM;

    switch (pmcr0 & CCM_PMCR0_PTVAI)
    {
    case CCM_PMCR0_PTVAI_DECREASE:
        wp++;
        dptc_nr_dn++;
        break;

    case CCM_PMCR0_PTVAI_INCREASE:
        wp--;
        dptc_nr_up++;
        break;

    case CCM_PMCR0_PTVAI_INCREASE_NOW:
        if (--wp > DPTC_WP_PANIC)
            wp = DPTC_WP_PANIC;
        dptc_nr_pnc++;
        break;

    case CCM_PMCR0_PTVAI_NO_INT:
        break; /* Just maintain at global level */
    }

    if (wp < 0)
        wp = 0;
    else if (wp >= DPTC_NUM_WP)
        wp = DPTC_NUM_WP - 1;

    entry = &dvfs_dptc_voltage_table[wp];

    sw1a = check_regulator_setting(entry->sw1a);
    sw1advs = check_regulator_setting(entry->sw1advs);
    sw1bdvs = check_regulator_setting(entry->sw1bdvs);
    sw1bstby = check_regulator_setting(entry->sw1bstby);

    switchers0 = dptc_reg_shadows[0] & ~(MC13783_SW1A | MC13783_SW1ADVS);
    dptc_regs_buf[0] = switchers0 |
                       sw1a << MC13783_SW1A_POS |         /* SW1A */
                       sw1advs << MC13783_SW1ADVS_POS;    /* SW1ADVS */
    switchers1 = dptc_reg_shadows[1] & ~(MC13783_SW1BDVS | MC13783_SW1BSTBY);
    dptc_regs_buf[1] = switchers1 |
                       sw1bdvs << MC13783_SW1BDVS_POS |   /* SW1BDVS */
                       sw1bstby << MC13783_SW1BSTBY_POS;  /* SW1BSTBY */

    dptc_wp = wp;

    mc13783_write_async(&dptc_pmic_xfer, dptc_pmic_regs,
                        dptc_regs_buf, 2, dptc_transfer_done_callback);
}


static void dptc_new_wp(unsigned int wp)
{
    dptc_wp = wp;
    /* "NO_INT" so the working point isn't incremented, just set. */
    dptc_int((CCM_PMCR0 & ~CCM_PMCR0_PTVAI) | CCM_PMCR0_PTVAI_NO_INT);
}


/* Interrupt vector for DPTC */
static __attribute__((interrupt("IRQ"))) void CCM_CLK_HANDLER(void)
{
    unsigned long pmcr0 = CCM_PMCR0;

    if ((pmcr0 & CCM_PMCR0_PTVAI) == CCM_PMCR0_PTVAI_NO_INT)
        dptc_nr_no++;

    dptc_int(pmcr0);
}


/* Initialize the DPTC hardware */
static void dptc_init(void)
{
    /* Force DPTC off if running for some reason. */
    imx31_regmod32(&CCM_PMCR0, CCM_PMCR0_PTVAIM,
                   CCM_PMCR0_PTVAIM | CCM_PMCR0_DPTEN);

     /* Shadow the regulator registers */
    mc13783_read_regs(dptc_pmic_regs, dptc_reg_shadows, 2);

    /* Set default, safe working point. */
    dptc_new_wp(DPTC_WP_DEFAULT);

    /* Interrupt goes to MCU, specified reference circuits enabled when
     * DPTC is active. */
    imx31_regset32(&CCM_PMCR0, CCM_PMCR0_PTVIS);

    imx31_regmod32(&CCM_PMCR0, DPTC_DRCE_MASK,
                   CCM_PMCR0_DRCE0 | CCM_PMCR0_DRCE1 |
                   CCM_PMCR0_DRCE2 | CCM_PMCR0_DRCE3);

    /* DPTC counting range = 256 system clocks */
    imx31_regclr32(&CCM_PMCR0, CCM_PMCR0_DCR);

    logf("DPTC: Initialized");
}


/* Start DPTC module */
static void dptc_start(void)
{
    int oldlevel = disable_irq_save();

    if (!dptc_running)
    {
        dptc_running = true;

        /* Enable DPTC and unmask interrupt. */
        avic_enable_int(INT_CCM_CLK, INT_TYPE_IRQ, INT_PRIO_DPTC, 
                        CCM_CLK_HANDLER);

        update_dptc_counts(dvfs_level, dptc_wp);
        enable_dptc();
    }

    restore_irq(oldlevel);

    logf("DPTC: started");
}


/* Stop the DPTC hardware if running and go back to default working point */
static void dptc_stop(void)
{
    int oldlevel = disable_irq_save();

    if (dptc_running)
    {
        dptc_running = false;

        /* Disable DPTC and mask interrupt. */
        CCM_PMCR0 = (CCM_PMCR0 & ~CCM_PMCR0_DPTEN) | CCM_PMCR0_PTVAIM;
        avic_disable_int(INT_CCM_CLK);

        /* Go back to default working point. */
        dptc_new_wp(DPTC_WP_DEFAULT);
    }

    restore_irq(oldlevel);

    logf("DPTC: stopped");
}


/** Main module interface **/

/* Initialize DVFS and DPTC */
void dvfs_dptc_init(void)
{
    dptc_init();
    dvfs_init();
}


/* Start DVFS and DPTC */
void dvfs_dptc_start(void)
{
    dvfs_start();
    dptc_start();
}


/* Stop DVFS and DPTC */
void dvfs_dptc_stop(void)
{
    dptc_stop();
    dvfs_stop();
}


/* Set a signal load tracking weight */
void dvfs_set_lt_weight(enum DVFS_LT_SIGS index, unsigned long value)
{
    volatile unsigned long *reg_p = &CCM_LTR2;
    unsigned int shift = 3 * index;

    if (index < 9)
    {
        reg_p = &CCM_LTR3;
        shift += 5; /* Bits 7:5, 10:8 ... 31:29 */
    }
    else if (index < 16)
    {
        shift -= 16; /* Bits 13:11, 16:14 ... 31:29 */
    }

    imx31_regmod32(reg_p, value << shift, 0x7 << shift);
}


/* Set a signal load detection mode */
void dvfs_set_lt_detect(enum DVFS_LT_SIGS index, bool edge)
{
    unsigned long bit = 0;

    if ((unsigned)index < 13)
        bit = 1ul << (index + 3);
    else if ((unsigned)index < 16)
        bit = 1ul << (index + 29);

    imx31_regmod32(&CCM_LTR0, edge ? bit : 0, bit);
}


void dvfs_set_gp_bit(enum DVFS_DVGPS dvgp, bool assert)
{
    if ((unsigned)dvgp <= 3)
    {
        unsigned long bit = 1ul << dvgp;
        imx31_regmod32(&CCM_PMCR1, assert ? bit : 0, bit);
    }
}


/* Turn the wait-for-interrupt monitoring on or off */
void dvfs_wfi_monitor(bool on)
{
    imx31_regmod32(&CCM_PMCR0, on ? 0 : CCM_PMCR0_WFIM,
                   CCM_PMCR0_WFIM);
}


/* Obtain the current core voltage setting, in millivolts 8-) */
unsigned int dvfs_dptc_get_voltage(void)
{
    unsigned int v;

    int oldlevel = disable_irq_save();
    v = dvfs_dptc_voltage_table[dptc_wp].sw[dvfs_level];
    restore_irq(oldlevel);

    /* 25mV steps from 0.900V to 1.675V */
    return v * 25 + 900;
}


/* Get the current DVFS level */
unsigned int dvfs_get_level(void)
{
    return dvfs_level;
}


/* If DVFS is disabled, set the level explicitly */
void dvfs_set_level(unsigned int level)
{
    int oldlevel = disable_irq_save();

    unsigned int currlevel =
        (CCM_PMCR0 & CCM_PMCR0_DVSUP) >> CCM_PMCR0_DVSUP_POS;

    if (!dvfs_running && level < DVFS_NUM_LEVELS && level != currlevel)
        set_current_dvfs_level(level);

    restore_irq(oldlevel);
}


/* Get the current DPTC working point */
unsigned int dptc_get_wp(void)
{
    return dptc_wp;
}


/* If DPTC is not running, set the working point explicitly */
void dptc_set_wp(unsigned int wp)
{
    int oldlevel = disable_irq_save();

    if (!dptc_running && wp < DPTC_NUM_WP)
        dptc_new_wp(wp);

    restore_irq(oldlevel);
}
