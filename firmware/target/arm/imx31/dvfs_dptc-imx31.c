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

/* Synchronize DPTC comparator value registers to new table row */
static void update_dptc_counts(void)
{
    const struct dptc_dcvr_table_entry * const entry =
        &dptc_dcvr_table[dvfs_level][dptc_wp];

    CCM_DCVR0 = entry->dcvr0;
    CCM_DCVR1 = entry->dcvr1;
    CCM_DCVR2 = entry->dcvr2;
    CCM_DCVR3 = entry->dcvr3;
}

/* Enable DPTC and unmask interrupt. */
static void enable_dptc(void)
{
    /* Enable DPTC, assert voltage change request. */
    CCM_PMCR0 = (CCM_PMCR0 & ~CCM_PMCR0_PTVAIM) | CCM_PMCR0_DPTEN |
                    CCM_PMCR0_DPVCR;
    udelay(2);
    /* Now set that voltage is valid */
    CCM_PMCR0 |= CCM_PMCR0_DPVV;
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

/* Wait for the UPDTEN flag to be set so that all bits may be written */
static inline void updten_wait(void)
{
    while (!(CCM_PMCR0 & CCM_PMCR0_UPDTEN));
}

/* Do the actual frequency and DVFS pin change - always call with IRQ masked */
static void do_dvfs_update(unsigned int level)
{
    const struct dvfs_clock_table_entry *setting = &dvfs_clock_table[level];
    unsigned long pmcr0 = CCM_PMCR0;

    if (pmcr0 & CCM_PMCR0_DPTEN)
    {
        /* Ignore voltage change request from DPTC. Voltage is *not* valid. */
        pmcr0 &= ~CCM_PMCR0_DPVCR;
        /* Mask DPTC interrupt for when called in thread context */
        pmcr0 |= CCM_PMCR0_PTVAIM;
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

    /* Save new level */
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
     * supposed to happen. Only do the lengthy delay with them enabled. */
    enable_irq();
    udelay(100); /* Software wait for voltage ramp-up */
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
        update_dptc_counts();
        enable_dptc();
    }
}

/* Start DVFS, change the set point and stop it */
static void set_current_dvfs_level(unsigned int level)
{
    int oldlevel;

    /* Have to wait at least 3 div3 clocks before enabling after being
     * stopped. */
    udelay(1500);

    oldlevel = disable_irq_save();
    CCM_PMCR0 |= CCM_PMCR0_DVFEN;
    do_dvfs_update(level);
    restore_irq(oldlevel);

    updten_wait();

    bitclr32(&CCM_PMCR0, CCM_PMCR0_DVFEN);
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

    do_dvfs_update(level);
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
static void INIT_ATTR dvfs_init(void)
{
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
    bitclr32(&GPIO1_GDIR, (1 << 5));
    iomuxc_set_pin_mux(IOMUXC_GPIO1_5,
                       IOMUXC_MUX_OUT_FUNCTIONAL | IOMUXC_MUX_IN_FUNCTIONAL);
#endif

    /* GP load bits disabled */
    bitclr32(&CCM_PMCR1, 0xf);
    
    /* Initialize DVFS signal weights and detection modes. */
    int i;
    for (i = 0; i < 16; i++)
    {
        dvfs_set_lt_weight(i, lt_signals[i].weight);
        dvfs_set_lt_detect(i, lt_signals[i].detect);
    }

    /* Set up LTR0. */
    bitmod32(&CCM_LTR0,
             DVFS_UPTHR << CCM_LTR0_UPTHR_POS |
             DVFS_DNTHR << CCM_LTR0_DNTHR_POS |
             DVFS_DIV3CK << CCM_LTR0_DIV3CK_POS,
             CCM_LTR0_UPTHR | CCM_LTR0_DNTHR | CCM_LTR0_DIV3CK);

    /* Set up LTR1. */
    bitmod32(&CCM_LTR1,
             DVFS_DNCNT << CCM_LTR1_DNCNT_POS |
             DVFS_UPCNT << CCM_LTR1_UPCNT_POS |
             DVFS_PNCTHR << CCM_LTR1_PNCTHR_POS |
             CCM_LTR1_LTBRSR,
             CCM_LTR1_DNCNT | CCM_LTR1_UPCNT |
             CCM_LTR1_PNCTHR | CCM_LTR1_LTBRSR);

    /* Set up LTR2-- EMA configuration. */
    bitmod32(&CCM_LTR2, DVFS_EMAC << CCM_LTR2_EMAC_POS, CCM_LTR2_EMAC);

    /* DVFS interrupt goes to MCU. Mask load buffer full interrupt. Do not
     * always give an event. */
    bitmod32(&CCM_PMCR0, CCM_PMCR0_DVFIS | CCM_PMCR0_LBMI,
             CCM_PMCR0_DVFIS | CCM_PMCR0_LBMI | CCM_PMCR0_DVFEV);

    /* Initialize current core PLL and dividers for default level. Assumes
     * clocking scheme has been set up appropriately in other init code. */
    ccm_set_mcupll_and_pdr(dvfs_clock_table[DVFS_LEVEL_DEFAULT].pll_val,
                           dvfs_clock_table[DVFS_LEVEL_DEFAULT].pdr_val);

    /* Set initial level and working point. */
    set_current_dvfs_level(DVFS_LEVEL_DEFAULT);

    logf("DVFS: Initialized");
}


/** DPTC **/

/* Request tracking since boot */
static bool dptc_running = false; /* Has driver enabled DPTC? */

unsigned int dptc_nr_dn = 0;
unsigned int dptc_nr_up = 0;
unsigned int dptc_nr_pnc = 0;
unsigned int dptc_nr_no = 0;

struct dptc_async_buf
{
    struct spi_transfer_desc xfer; /* transfer descriptor */
    unsigned int wp;               /* new working point */
    uint32_t buf[2];               /* buffer for async write */
};

static struct dptc_async_buf dptc_async_buf;    /* ISR async write buffer */
static const unsigned char dptc_pmic_regs[2] =  /* Register subaddresses */
    { MC13783_SWITCHERS0, MC13783_SWITCHERS1 };
static uint32_t dptc_reg_shadows[2];            /* shadow regs */

/* Called (in SPI INT context) after asynchronous PMIC write is completed */
static void dptc_transfer_done_callback(struct spi_transfer_desc *xfer)
{
    if (xfer->count != 0)
        return;

    /* Save new working point */
    dptc_wp = ((struct dptc_async_buf *)xfer)->wp;

    update_dptc_counts();

    if (dptc_running)
        enable_dptc();
}

/* Handle the DPTC interrupt and sometimes the manual setting - always call
 * with IRQ masked. */
static void dptc_int(unsigned long pmcr0, int wp, struct dptc_async_buf *abuf)
{
    const union dvfs_dptc_voltage_table_entry *entry;
    uint32_t sw1a, sw1advs, sw1bdvs, sw1bstby;
    uint32_t switchers0, switchers1;

    /* Mask DPTC interrupt and disable DPTC until the change request is
     * serviced. */
    CCM_PMCR0 = (pmcr0 & ~CCM_PMCR0_DPTEN) | CCM_PMCR0_PTVAIM;

    switch (pmcr0 & CCM_PMCR0_PTVAI)
    {
    case CCM_PMCR0_PTVAI_DECREASE:
        /* Decrease voltage request - increment working point */
        wp++;
        dptc_nr_dn++;
        break;

    case CCM_PMCR0_PTVAI_INCREASE:
        /* Increase voltage request - decrement working point */
        wp--;
        dptc_nr_up++;
        break;

    case CCM_PMCR0_PTVAI_INCREASE_NOW:
        /* Panic request - move immediately to panic working point if
         * decrement results in greater working point than DPTC_WP_PANIC. */
        if (--wp > DPTC_WP_PANIC)
            wp = DPTC_WP_PANIC;
        dptc_nr_pnc++;
        break;

    case CCM_PMCR0_PTVAI_NO_INT:
        /* Just maintain at global level */
        if (abuf == &dptc_async_buf)
            dptc_nr_no++;
        break;
    }

    /* Keep result in range */
    if (wp < 0)
        wp = 0;
    else if (wp >= DPTC_NUM_WP)
        wp = DPTC_NUM_WP - 1;

    /* Get new regulator register settings, sanity check them and write them
     * in the background. */
    entry = &dvfs_dptc_voltage_table[wp];

    sw1a = check_regulator_setting(entry->sw1a);
    sw1advs = check_regulator_setting(entry->sw1advs);
    sw1bdvs = check_regulator_setting(entry->sw1bdvs);
    sw1bstby = check_regulator_setting(entry->sw1bstby);

    switchers0 = dptc_reg_shadows[0] & ~(MC13783_SW1A | MC13783_SW1ADVS);
    abuf->buf[0] = switchers0 |
                   sw1a << MC13783_SW1A_POS |         /* SW1A */
                   sw1advs << MC13783_SW1ADVS_POS;    /* SW1ADVS */
    switchers1 = dptc_reg_shadows[1] & ~(MC13783_SW1BDVS | MC13783_SW1BSTBY);
    abuf->buf[1] = switchers1 |
                   sw1bdvs << MC13783_SW1BDVS_POS |   /* SW1BDVS */
                   sw1bstby << MC13783_SW1BSTBY_POS;  /* SW1BSTBY */

    abuf->wp = wp; /* Save new for xfer completion handler */
    mc13783_write_async(&abuf->xfer, dptc_pmic_regs, abuf->buf, 2,
                        dptc_transfer_done_callback);
}

/* Handle setting the working point explicitly - always call with IRQ
 * masked */
static void dptc_new_wp(unsigned int wp)
{
    struct dptc_async_buf buf;

    /* "NO_INT" so the working point isn't incremented, just set. */
    dptc_int(CCM_PMCR0 & ~CCM_PMCR0_PTVAI, wp, &buf);

    /* Wait for PMIC write */
    while (!spi_transfer_complete(&buf.xfer))
    {
        enable_irq();
        nop; nop; nop; nop; nop;
        disable_irq();
    }
}

/* Interrupt vector for DPTC */
static __attribute__((interrupt("IRQ"))) void CCM_CLK_HANDLER(void)
{
    dptc_int(CCM_PMCR0, dptc_wp, &dptc_async_buf);
}

/* Initialize the DPTC hardware */
static void INIT_ATTR dptc_init(void)
{
    int oldlevel;

    /* Shadow the regulator registers */
    mc13783_read_regs(dptc_pmic_regs, dptc_reg_shadows, 2);

    /* Set default, safe working point. */
    oldlevel = disable_irq_save();
    dptc_new_wp(DPTC_WP_DEFAULT);
    restore_irq(oldlevel);

    /* Interrupt goes to MCU, specified reference circuits enabled when
     * DPTC is active, DPTC counting range = 256 system clocks */
    bitmod32(&CCM_PMCR0,
             CCM_PMCR0_PTVIS | DPTC_DRCE_MASK,
             CCM_PMCR0_PTVIS | CCM_PMCR0_DCR |
             CCM_PMCR0_DRCE0 | CCM_PMCR0_DRCE1 |
             CCM_PMCR0_DRCE2 | CCM_PMCR0_DRCE3);

    logf("DPTC: Initialized");
}


/** Main module interface **/

/** DVFS+DPTC **/

/* Initialize DVFS and DPTC */
void INIT_ATTR dvfs_dptc_init(void)
{
    /* DVFS or DPTC on for some reason? Force off. */
    bitmod32(&CCM_PMCR0,
             CCM_PMCR0_FSVAIM | CCM_PMCR0_LBMI |
             CCM_PMCR0_PTVAIM,
             CCM_PMCR0_FSVAIM | CCM_PMCR0_LBMI | CCM_PMCR0_DVFEN |
             CCM_PMCR0_PTVAIM | CCM_PMCR0_DPTEN);

    /* Ensure correct order - after this, the two appear independent */
    dptc_init();
    dvfs_init();
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


/** DVFS **/

/* Start the DVFS hardware */
void dvfs_start(void)
{
    if (dvfs_running)
        return;

    /* Have to wait at least 3 div3 clocks before enabling after being
     * stopped. */
    udelay(1500);

    /* Unmask DVFS interrupt source and enable DVFS. */
    bitmod32(&CCM_PMCR0, CCM_PMCR0_DVFEN,
             CCM_PMCR0_FSVAIM | CCM_PMCR0_DVFEN);

    dvfs_running = true;

    avic_enable_int(INT_CCM_DVFS, INT_TYPE_IRQ, INT_PRIO_DVFS,
                    CCM_DVFS_HANDLER);

    logf("DVFS: started");
}

/* Stop the DVFS hardware and return to default frequency */
void dvfs_stop(void)
{
    if (!dvfs_running)
        return;

    /* Mask DVFS interrupts. */
    avic_disable_int(INT_CCM_DVFS);
    bitset32(&CCM_PMCR0, CCM_PMCR0_FSVAIM | CCM_PMCR0_LBMI);

    if (((CCM_PMCR0 & CCM_PMCR0_DVSUP) >> CCM_PMCR0_DVSUP_POS) !=
            DVFS_LEVEL_DEFAULT)
    {
        int oldlevel;
        /* Set default frequency level */
        updten_wait();
        oldlevel = disable_irq_save();
        do_dvfs_update(DVFS_LEVEL_DEFAULT);
        restore_irq(oldlevel);
        updten_wait();
    }

    /* Disable DVFS. */
    bitclr32(&CCM_PMCR0, CCM_PMCR0_DVFEN);
    dvfs_running = false;

    logf("DVFS: stopped");
}

/* Is DVFS enabled? */
bool dvfs_enabled(void)
{
   return dvfs_running; 
}

/* If DVFS is disabled, set the level explicitly */
void dvfs_set_level(unsigned int level)
{
    if (dvfs_running ||
        level >= DVFS_NUM_LEVELS ||
        !((1 << level) & DVFS_LEVEL_MASK) ||
        level == ((CCM_PMCR0 & CCM_PMCR0_DVSUP) >> CCM_PMCR0_DVSUP_POS))
        return;

    set_current_dvfs_level(level);
}

/* Get the current DVFS level */
unsigned int dvfs_get_level(void)
{
    return dvfs_level;
}

/* Get bitmask of levels supported */
unsigned int dvfs_level_mask(void)
{
    return DVFS_LEVEL_MASK;
}

/* Mask the DVFS interrupt without affecting running status */
void dvfs_int_mask(bool mask)
{
    if (mask)
    {
        /* Just disable, not running = already disabled */
        avic_mask_int(INT_CCM_DVFS);
    }
    else if (dvfs_running)
    {
        /* DVFS is running; unmask it */
        avic_unmask_int(INT_CCM_DVFS);
    }
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

    bitmod32(reg_p, value << shift, 0x7 << shift);
}

/* Return a signal load tracking weight */
unsigned long dvfs_get_lt_weight(enum DVFS_LT_SIGS index)
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

    return (*reg_p & (0x7 << shift)) >> shift;
}

/* Set a signal load detection mode */
void dvfs_set_lt_detect(enum DVFS_LT_SIGS index, bool edge)
{
    unsigned long bit = 0;

    if ((unsigned)index < 13)
        bit = 1ul << (index + 3);
    else if ((unsigned)index < 16)
        bit = 1ul << (index + 29);

    bitmod32(&CCM_LTR0, edge ? bit : 0, bit);
}

/* Returns a signal load detection mode */
bool dvfs_get_lt_detect(enum DVFS_LT_SIGS index)
{
    unsigned int shift = 32;

    if ((unsigned)index < 13)
        shift = index + 3;
    else if ((unsigned)index < 16)
        shift = index + 29;

    return !!((CCM_LTR0 & (1ul << shift)) >> shift);
}

/* Set/clear the general-purpose load tracking bit */
void dvfs_set_gp_bit(enum DVFS_DVGPS dvgp, bool assert)
{
    if ((unsigned)dvgp <= 3)
    {
        unsigned long bit = 1ul << dvgp;
        bitmod32(&CCM_PMCR1, assert ? bit : 0, bit);
    }
}

/* Return the general-purpose load tracking bit */
bool dvfs_get_gp_bit(enum DVFS_DVGPS dvgp)
{
    if ((unsigned)dvgp <= 3)
        return (CCM_PMCR1 & (1ul << dvgp)) != 0;
    return false;
}

/* Set GP load tracking by code.
 * level_code:
 *     lt 0  =defaults
 *     0     =all off ->
 *     28    =highest load
 *     gte 28=highest load
 * detect_mask bits:
 *     b[3:0]: 1=LTn edge detect, 0=LTn level detect
 */
void dvfs_set_gp_sense(int level_code, unsigned long detect_mask)
{
    int i;

    for (i = 0; i <= 3; i++)
    {
        int ltsig_num = DVFS_LT_SIG_DVGP0 + i;
        int gpw_num = DVFS_DVGP_0 + i;
        unsigned long weight;
        bool edge;
        bool assert;

        if (level_code < 0)
        {
            /* defaults */
            detect_mask = 0;
            assert = 0;
            weight = lt_signals[ltsig_num].weight;
            edge = lt_signals[ltsig_num].detect != 0;
        }
        else
        {
            weight = MIN(level_code, 7);
            edge = !!(detect_mask & 1);
            assert = weight > 0;
            detect_mask >>= 1;
            level_code -= 7;
            if (level_code < 0)
                level_code = 0;
        }

        dvfs_set_lt_weight(ltsig_num, weight);  /* set weight */
        dvfs_set_lt_detect(ltsig_num, edge);    /* set detect mode */
        dvfs_set_gp_bit(gpw_num, assert);       /* set activity */
    }
}

/* Return GP weight settings */
void dvfs_get_gp_sense(int *level_code, unsigned long *detect_mask)
{
    int i;
    int code = 0;
    unsigned long mask = 0;

    for (i = DVFS_LT_SIG_DVGP0; i <= DVFS_LT_SIG_DVGP3; i++)
    {
        code += dvfs_get_lt_weight(i);
        mask = (mask << 1) | (dvfs_get_lt_detect(i) ? 1 : 0);
    }

    if (level_code)
        *level_code = code;

    if (detect_mask)
        *detect_mask = mask;
}

/* Turn the wait-for-interrupt monitoring on or off */
void dvfs_wfi_monitor(bool on)
{
    bitmod32(&CCM_PMCR0, on ? 0 : CCM_PMCR0_WFIM, CCM_PMCR0_WFIM);
}


/** DPTC **/

/* Start DPTC module */
void dptc_start(void)
{
    int oldlevel;

    if (dptc_running)
        return;

    oldlevel = disable_irq_save();
    dptc_running = true;

    /* Enable DPTC and unmask interrupt. */
    update_dptc_counts();
    enable_dptc();
    restore_irq(oldlevel);

    avic_enable_int(INT_CCM_CLK, INT_TYPE_IRQ, INT_PRIO_DPTC, 
                    CCM_CLK_HANDLER);

    logf("DPTC: started");
}

/* Stop the DPTC hardware if running and go back to default working point */
void dptc_stop(void)
{
    int oldlevel;

    if (!dptc_running)
        return;

    avic_disable_int(INT_CCM_CLK);

    oldlevel = disable_irq_save();
    dptc_running = false;

    /* Disable DPTC and mask interrupt. */
    CCM_PMCR0 = (CCM_PMCR0 & ~CCM_PMCR0_DPTEN) | CCM_PMCR0_PTVAIM;

    /* Go back to default working point. */
    dptc_new_wp(DPTC_WP_DEFAULT);
    restore_irq(oldlevel);

    logf("DPTC: stopped");
}

/* Is DPTC enabled? */
bool dptc_enabled(void)
{
    return dptc_running;
}

/* If DPTC is not running, set the working point explicitly */
void dptc_set_wp(unsigned int wp)
{
    if (!dptc_running && wp < DPTC_NUM_WP)
        dptc_new_wp(wp);
}

/* Get the current DPTC working point */
unsigned int dptc_get_wp(void)
{
    return dptc_wp;
}
