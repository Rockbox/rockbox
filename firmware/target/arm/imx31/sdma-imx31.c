/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sevakis
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
#include <string.h>
#include "logf.h"
#include "panic.h"
#include "clkctl-imx31.h"
#include "avic-imx31.h"
#include "sdma_struct.h"
#include "sdma-imx31.h"
#include "sdma_script_code.h"
#include "mmu-imx31.h"

/* Most of the code in here is based upon the Linux BSP provided by Freescale
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved. */

/* Cut down to bare bones essentials */

/* Script information that depends on system revision */
static struct sdma_script_start_addrs script_info;
/* Mask of channels with callback enabled */
static unsigned long sdma_enabled_ints = 0;
/* One channel control block per channel in physically mapped device RAM */
static struct channel_control_block ccb_array[CH_NUM] DEVBSS_ATTR;
/* Channel 0 (command channel) data */
static struct buffer_descriptor_extd c0_buffer_desc DEVBSS_ATTR;

/* All SDMA channel interrupts are handled here.
 * Dispatches lower channel numbers first (prioritized by SDMA API callers
 * who specify the desired channel number).
 */
static void __attribute__((interrupt("IRQ"))) SDMA_HANDLER(void)
{
    unsigned long pending = SDMA_INTR;

    SDMA_INTR = pending;          /* Ack all ints */
    pending &= sdma_enabled_ints; /* Only dispatch ints with callback */

    while (1)
    {
        unsigned int channel;

        if (pending == 0)
            break; /* No bits set */

        channel = find_first_set_bit(pending);

        pending &= ~(1ul << channel);

        /* Call callback (required if using an interrupt) */
        ccb_array[channel].channel_desc->callback();
    }
}

/* Initialize script information based upon the system revision */
static void init_script_info(void)
{
    if (iim_system_rev() == IIM_SREV_1_0)
    {
        /* Channel script info */
        script_info.app_2_mcu_addr = app_2_mcu_ADDR_1;
        script_info.ap_2_ap_addr = ap_2_ap_ADDR_1;
        script_info.ap_2_bp_addr = -1;
        script_info.bp_2_ap_addr = -1;
        script_info.loopback_on_dsp_side_addr = -1;
        script_info.mcu_2_app_addr = mcu_2_app_ADDR_1;
        script_info.mcu_2_shp_addr = mcu_2_shp_ADDR_1;
        script_info.mcu_interrupt_only_addr = -1;
        script_info.shp_2_mcu_addr = shp_2_mcu_ADDR_1;
        script_info.uartsh_2_mcu_addr = uartsh_2_mcu_ADDR_1;
        script_info.uart_2_mcu_addr = uart_2_mcu_ADDR_1;
        script_info.dptc_dvfs_addr = dptc_dvfs_ADDR_1;
        script_info.firi_2_mcu_addr = firi_2_mcu_ADDR_1;
        script_info.firi_2_per_addr = -1;
        script_info.mshc_2_mcu_addr = mshc_2_mcu_ADDR_1;
        script_info.per_2_app_addr = -1;
        script_info.per_2_firi_addr = -1;
        script_info.per_2_shp_addr = -1;
        script_info.mcu_2_ata_addr = mcu_2_ata_ADDR_1;
        script_info.mcu_2_firi_addr = mcu_2_firi_ADDR_1;
        script_info.mcu_2_mshc_addr = mcu_2_mshc_ADDR_1;
        script_info.ata_2_mcu_addr = ata_2_mcu_ADDR_1;
        script_info.uartsh_2_per_addr = -1;
        script_info.shp_2_per_addr = -1;
        script_info.uart_2_per_addr = -1;
        script_info.app_2_per_addr = -1;
        /* Main code block info */
        script_info.ram_code_size = RAM_CODE_SIZE_1;
        script_info.ram_code_start_addr = RAM_CODE_START_ADDR_1;
        script_info.mcu_start_addr = (unsigned long)sdma_code_1;
    }
    else
    {
        /* Channel script info */
        script_info.app_2_mcu_addr = app_2_mcu_patched_ADDR_2;
        script_info.ap_2_ap_addr = ap_2_ap_ADDR_2;
        script_info.ap_2_bp_addr = ap_2_bp_ADDR_2;
        script_info.bp_2_ap_addr = bp_2_ap_ADDR_2;
        script_info.loopback_on_dsp_side_addr = -1;
        script_info.mcu_2_app_addr = mcu_2_app_patched_ADDR_2;
        script_info.mcu_2_shp_addr = mcu_2_shp_patched_ADDR_2;
        script_info.mcu_interrupt_only_addr = -1;
        script_info.shp_2_mcu_addr = shp_2_mcu_patched_ADDR_2;
        script_info.uartsh_2_mcu_addr = uartsh_2_mcu_patched_ADDR_2;
        script_info.uart_2_mcu_addr = uart_2_mcu_patched_ADDR_2;
        script_info.dptc_dvfs_addr = -1;
        script_info.firi_2_mcu_addr = firi_2_mcu_ADDR_2;
        script_info.firi_2_per_addr = -1;
        script_info.mshc_2_mcu_addr = -1;
        script_info.per_2_app_addr = -1;
        script_info.per_2_firi_addr = -1;
        script_info.per_2_shp_addr = per_2_shp_ADDR_2;
        script_info.mcu_2_ata_addr = mcu_2_ata_ADDR_2;
        script_info.mcu_2_firi_addr = mcu_2_firi_ADDR_2;
        script_info.mcu_2_mshc_addr = -1;
        script_info.ata_2_mcu_addr = ata_2_mcu_ADDR_2;
        script_info.uartsh_2_per_addr = -1;
        script_info.shp_2_per_addr = shp_2_per_ADDR_2;
        script_info.uart_2_per_addr = -1;
        script_info.app_2_per_addr = -1;
        /* Main code block info */
        script_info.ram_code_size = RAM_CODE_SIZE_2;
        script_info.ram_code_start_addr = RAM_CODE_START_ADDR_2;
        script_info.mcu_start_addr = (unsigned long)sdma_code_2;
    }
}

/* Return pc of SDMA script in SDMA halfword space according to peripheral
 * and transfer type */
static unsigned long get_script_pc(unsigned int peripheral_type,
                                   unsigned int transfer_type)
{
    unsigned long res = (unsigned short)-1;

    switch (peripheral_type)
    {
    case SDMA_PER_MEMORY:
        switch (transfer_type)
        {
        case SDMA_TRAN_EMI_2_INT:
        case SDMA_TRAN_EMI_2_EMI:
        case SDMA_TRAN_INT_2_EMI:
            res = script_info.ap_2_ap_addr;
            break;
        }
        break;

#if 0 /* Not using this */
    case SDMA_PER_DSP:
        switch (transfer_type)
        {
        case SDMA_TRAN_EMI_2_DSP:
            res = script_info.ap_2_bp_addr;
            break;
        case SDMA_TRAN_DSP_2_EMI:
            res = script_info.bp_2_ap_addr;
            break;
        case SDMA_TRAN_DSP_2_EMI_LOOP:
            res = script_info.loopback_on_dsp_side_addr;
            break;
        case SDMA_TRAN_EMI_2_DSP_LOOP:
            res = script_info.mcu_interrupt_only_addr;
            break;
        }
        break;
#endif

#if 0 /* Not using this */
    case SDMA_PER_FIRI:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = script_info.firi_2_per_addr;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.firi_2_mcu_addr;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = script_info.per_2_firi_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_firi_addr;
            break;
        }
        break;
#endif

#if 0 /* Not using this */
    case SDMA_PER_UART:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = script_info.uart_2_per_addr;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.uart_2_mcu_addr;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = script_info.per_2_app_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_app_addr;
            break;
        }
        break;
#endif

#if 0 /* Not using this */
    case SDMA_PER_UART_SHP:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = script_info.uartsh_2_per_addr;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.uartsh_2_mcu_addr;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = script_info.per_2_shp_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_shp_addr;
            break;
        }
        break;
#endif

    case SDMA_PER_ATA:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.ata_2_mcu_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_ata_addr;
            break;
        }
        break;

    case SDMA_PER_CSPI:
    case SDMA_PER_EXT:
    case SDMA_PER_SSI:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = script_info.app_2_per_addr;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.app_2_mcu_addr;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = script_info.per_2_app_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_app_addr;
            break;
        }
        break;

#if 0 /* Not using this */
    case SDMA_PER_MMC:
    case SDMA_PER_SDHC:
#endif
    case SDMA_PER_SSI_SHP:
    case SDMA_PER_CSPI_SHP:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = script_info.shp_2_per_addr;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.shp_2_mcu_addr;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = script_info.per_2_shp_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_shp_addr;
            break;
        }
        break;

    case SDMA_PER_MSHC:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.mshc_2_mcu_addr;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = script_info.mcu_2_mshc_addr;
            break;
        }
        break;

    case SDMA_PER_CCM:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_EMI:
            res = script_info.dptc_dvfs_addr;
            break;
        }
        break;
    }

    if (res == (unsigned short)-1)
    {
        logf("SDMA script not found\n");
    }

    return res;
}

static unsigned int get_config(unsigned int transfer_type)
{
    unsigned int res = -1;

    switch (transfer_type)
    {
    case SDMA_TRAN_PER_2_INT:
    case SDMA_TRAN_PER_2_EMI:
    case SDMA_TRAN_INT_2_PER:
    case SDMA_TRAN_EMI_2_PER:
        /*
         * Peripheral <------> Memory
         * evtOvr = 0 mcuOvr = 0 dspOvr = 1
         */
        res = CH_OWNSHP_MCU | CH_OWNSHP_EVT;
        break;

#if 0 /* Not using this */
    case SDMA_TRAN_DSP_2_PER:
        res = 0;
        break;
    case SDMA_TRAN_EMI_2_DSP:
    case SDMA_TRAN_INT_2_DSP:
    case SDMA_TRAN_DSP_2_INT:
    case SDMA_TRAN_DSP_2_EMI:
    case SDMA_TRAN_DSP_2_DSP:
        /*
         * DSP <-----------> Memory
         * evtOvr = 1 mcuOvr = 0 dspOvr = 0
         */
        res = CH_OWNSHP_MCU | CH_OWNSHP_DSP;
        break;
#endif

    case SDMA_TRAN_EMI_2_INT:
    case SDMA_TRAN_EMI_2_EMI:
    case SDMA_TRAN_INT_2_INT:
    case SDMA_TRAN_INT_2_EMI:
#if 0 /* Not using this */
    case SDMA_TRAN_DSP_2_EMI_LOOP:
    case SDMA_TRAN_EMI_2_DSP_LOOP:
#endif
        /* evtOvr = 1 mcuOvr = 0 dspOvr = 1 */
        res = CH_OWNSHP_MCU;
        break;

#if 0 /* Not using this */
    case SDMA_TRAN_PER_2_DSP:
        /* evtOvr = 0 mcuOvr = 1 dspOvr = 0 */
        res = CH_OWNSHP_DSP | CH_OWNSHP_EVT;
        break;
#endif

    default:
        break;
    }

    return res;
}

/* Fill the buffer descriptor with the values given in parameter.
 * Expects physical addresses. */
static inline void set_buffer_descriptor(
    struct buffer_descriptor *bd_p,
    unsigned int command, /* C0_* command or transfer size */
    unsigned int status,  /* BD_* flags */
    unsigned int count,   /* Size of buffer to transfer */
    void *buf_addr,       /* Buffer to transfer */
    void *buf_addr_ext)
{
    bd_p->mode.command = command;
    bd_p->mode.status = status;
    bd_p->mode.count = count;
    bd_p->buf_addr = buf_addr;
    if (status & BD_EXTD)
        ((struct buffer_descriptor_extd *)bd_p)->buf_addr_ext = buf_addr_ext;
}

/* Configure channel ownership */
static void set_channel_ownership(unsigned int channel, unsigned int config)
{
    unsigned long bit = 1ul << channel;

    /* DSP side */
#if 0 /* Not using this */
    imx31_regmod32(&SDMA_DSPOVR, (config & CH_OWNSHP_DSP) ? 0 : bit, bit);
#endif
    /* Event */
    imx31_regmod32(&SDMA_EVTOVR, (config & CH_OWNSHP_EVT) ? 0 : bit, bit);
    /* MCU side */
    imx31_regmod32(&SDMA_HOSTOVR, (config & CH_OWNSHP_MCU) ? 0 : bit, bit);
}

static bool setup_channel(struct channel_control_block *ccb_p)
{
    struct context_data context_buffer;
    struct channel_descriptor *cd_p;
    unsigned int channel_cfg;
    unsigned int channel;
    unsigned long pc;

    memset(&context_buffer, 0x00, sizeof (context_buffer));

    channel = ccb_p - ccb_array;
    cd_p = ccb_p->channel_desc;

    /* Obtain script start address for perihperal and transfer type */
    pc = get_script_pc(cd_p->per_type, cd_p->tran_type);

    if (pc == (unsigned short)-1)
        return false; /* Failed to find a script */

    context_buffer.channel_state.pc = pc;

    if (cd_p->per_type != SDMA_PER_MEMORY && cd_p->per_type != SDMA_PER_DSP)
    {
        /* Set peripheral DMA request mask for this channel */
        context_buffer.event_mask1 = 1ul << cd_p->event_id1;

        if (cd_p->per_type == SDMA_PER_ATA)
        {
            /* ATA has two */
            context_buffer.event_mask2 = 1ul << cd_p->event_id2;
        }

        context_buffer.shp_addr = cd_p->shp_addr;
        context_buffer.wml = cd_p->wml;
    }
    else
    {
        context_buffer.wml = SDMA_PER_ADDR_SDRAM;
    }

    /* Send channel context to SDMA core */
    clean_dcache_range(&context_buffer, sizeof (context_buffer));
    sdma_write_words((unsigned long *)&context_buffer,
                     CHANNEL_CONTEXT_ADDR(channel),
                     sizeof (context_buffer)/4);

    if (cd_p->is_setup != 0)
        return true; /* No more to do */

    /* Obtain channel ownership configuration */
    channel_cfg = get_config(cd_p->tran_type);

    if (channel_cfg == (unsigned int)-1)
        return false;

    /* Set who owns it and thus can activate it */
    set_channel_ownership(channel, channel_cfg);

    if (channel_cfg & CH_OWNSHP_EVT)
    {
        /* Set event ID to channel activation bitmapping */
        imx31_regset32(&SDMA_CHNENBL(cd_p->event_id1), 1ul << channel);

        if (cd_p->per_type == SDMA_PER_ATA)
        {
            /* ATA has two */
            imx31_regset32(&SDMA_CHNENBL(cd_p->event_id2), 1ul << channel);
        }
    }

    cd_p->is_setup = 1;

    return true;
}

/** Public routines **/
void sdma_init(void)
{
    imx31_clkctl_module_clock_gating(CG_SDMA, CGM_ON_RUN_WAIT);
    int i;
    unsigned long acr;

    /* Reset the controller */
    SDMA_RESET |= SDMA_RESET_RESET;
    while (SDMA_RESET & SDMA_RESET_RESET);

    init_script_info();

    /* No channel enabled, all priorities 0 */
    for (i = 0; i < CH_NUM; i++)
    {
        SDMA_CHNENBL(i) = 0;
        SDMA_CHNPRI(i) = 0;
    }

    /* Ensure no ints pending */
    SDMA_INTR = 0xffffffff;

    /* Nobody owns any channel (yet) */
    SDMA_HOSTOVR = 0xffffffff;
    SDMA_DSPOVR = 0xffffffff;
    SDMA_EVTOVR = 0xffffffff;

    SDMA_MC0PTR = 0x00000000;

    /* 32-word channel contexts, use default bootscript address */
    SDMA_CHN0ADDR = SDMA_CHN0ADDR_SMSZ | 0x0050;

    avic_enable_int(SDMA, IRQ, 8, SDMA_HANDLER);

    /* SDMA core must run at the proper frequency based upon the AHB/IPG ratio */
    acr = (imx31_clkctl_get_ahb_clk() / imx31_clkctl_get_ipg_clk()) < 2 ?
          SDMA_CONFIG_ACR : 0;

    /* No dsp, no debug
     * Static context switching - TLSbo86520L SW Workaround for SDMA Chnl0
     * access issue */
    SDMA_CONFIG = acr;

    /* Tell SDMA where the host channel table is */
    SDMA_MC0PTR = (unsigned long)ccb_array;

    ccb_array[0].status.opened_init = 1;
    ccb_array[0].curr_bd_ptr = &c0_buffer_desc.bd;
    ccb_array[0].base_bd_ptr = &c0_buffer_desc.bd;
    ccb_array[0].channel_desc = NULL; /* No channel descriptor */

    /* Command channel owned by AP */
    set_channel_ownership(0, CH_OWNSHP_MCU);

    sdma_channel_set_priority(0, 1);

    /* Load SDMA script code */
    set_buffer_descriptor(&c0_buffer_desc.bd,
                          C0_SETPM,
                          BD_DONE | BD_WRAP | BD_EXTD,
                          script_info.ram_code_size,
                          (void *)addr_virt_to_phys(script_info.mcu_start_addr),
                          (void *)(unsigned long)script_info.ram_code_start_addr);

    SDMA_HSTART = 1ul;
    sdma_channel_wait_nonblocking(0);

    /* No dsp, no debug, dynamic context switching */
    SDMA_CONFIG = acr | SDMA_CONFIG_CSM_DYNAMIC;
}

/* Busy wait for a channel to complete */
void sdma_channel_wait_nonblocking(unsigned int channel)
{
    unsigned long mask;

    if (channel >= CH_NUM)
        return;

    if (ccb_array[channel].status.opened_init == 0)
        return;

    mask = 1ul << channel;
    while (SDMA_STOP_STAT & mask);
}

/* Set a new channel priority */
void sdma_channel_set_priority(unsigned int channel, unsigned int priority)
{
    if (channel >= CH_NUM || priority > MAX_CH_PRIORITY)
        return;

    if (ccb_array[channel].status.opened_init == 0)
        return;

    SDMA_CHNPRI(channel) = priority;
}

/* Resets a channel to start of script next time it runs. */
bool sdma_channel_reset(unsigned int channel)
{
    struct channel_control_block *ccb_p;

    if (channel == 0 || channel >= CH_NUM)
        return false;

    ccb_p = &ccb_array[channel];

    if (ccb_p->status.opened_init == 0)
        return false;

    if (!setup_channel(ccb_p))
        return false;

    return true;
}

/* Resume or start execution on a channel */
void sdma_channel_run(unsigned int channel)
{
    if (channel == 0 || channel >= CH_NUM)
        return;

    if (ccb_array[channel].status.opened_init == 0)
        return;

    SDMA_HSTART = 1ul << channel;
}

/* Pause a running channel - can be resumed */
void sdma_channel_pause(unsigned int channel)
{
    if (channel == 0 || channel >= CH_NUM)
        return;

    if (ccb_array[channel].status.opened_init == 0)
        return;

    SDMA_STOP_STAT = 1ul << channel;
}

/* Stop a channel from executing - cannot be resumed */
void sdma_channel_stop(unsigned int channel)
{
    struct channel_control_block *ccb_p;
    unsigned long chmsk;
    unsigned long intmsk;
    int oldstatus;
    int i;

    if (channel == 0 || channel >= CH_NUM)
        return;

    ccb_p = &ccb_array[channel];

    if (ccb_p->status.opened_init == 0)
        return;

    chmsk = 1ul << channel;

    /* Lock callback */
    oldstatus = disable_irq_save();    
    intmsk = sdma_enabled_ints;
    sdma_enabled_ints &= ~chmsk;
    restore_irq(oldstatus);

    /* Stop execution */
    for (i = ccb_p->channel_desc->bd_count - 1; i >= 0; i--)
        ccb_p->base_bd_ptr[i].mode.status &= ~BD_DONE;

    SDMA_STOP_STAT = chmsk;
    while (SDMA_STOP_STAT & chmsk);

    /* Unlock callback if it was set */
    if (intmsk & chmsk)
        imx31_regset32(&sdma_enabled_ints, chmsk);

    logf("SDMA ch closed: %d", channel);
}

bool sdma_channel_init(unsigned int channel,
                       struct channel_descriptor *cd_p,
                       struct buffer_descriptor *base_bd_p)
{
    struct channel_control_block *ccb_p;

    if (channel == 0 || channel >= CH_NUM ||
        cd_p == NULL || base_bd_p == NULL)
        return false;

    ccb_p = &ccb_array[channel];

    /* If initialized already, should close first then init. */
    if (ccb_p->status.opened_init != 0)
        return false;

    /* Initialize channel control block. */
    ccb_p->curr_bd_ptr = base_bd_p;
    ccb_p->base_bd_ptr = base_bd_p;
    ccb_p->channel_desc = cd_p;
    ccb_p->status.error = 0;
    ccb_p->status.opened_init = 1;
    ccb_p->status.state_direction = 0;
    ccb_p->status.execute = 0;

    /* Finish any channel descriptor inits. */
    cd_p->ccb_ptr = ccb_p;
    cd_p->is_setup = 0;

    /* Do an initial setup now. */
    if (!setup_channel(ccb_p))
    {
        logf("SDMA ch init failed: %d", channel);
        cd_p->ccb_ptr = NULL;
        memset(ccb_p, 0x00, sizeof (struct channel_control_block));
        return false;
    }

    /* Enable interrupt if a callback is specified. */
    if (cd_p->callback != NULL)
        imx31_regset32(&sdma_enabled_ints, 1ul << channel);

    /* Minimum schedulable = 1 */
    sdma_channel_set_priority(channel, 1);

    logf("SDMA ch initialized: %d", channel);
    return true;
}

void sdma_channel_close(unsigned int channel)
{
    struct channel_control_block *ccb_p;
    int i;

    if (channel == 0 || channel >= CH_NUM)
        return;

    ccb_p = &ccb_array[channel];

    /* Block callbacks (if not initialized, it won't be set). */
    imx31_regclr32(&sdma_enabled_ints, 1ul << channel);

    if (ccb_p->status.opened_init == 0)
        return;

    /* Stop the channel if running */
    for (i = ccb_p->channel_desc->bd_count - 1; i >= 0; i--)
        ccb_p->base_bd_ptr[i].mode.status &= ~BD_DONE;

    sdma_channel_stop(channel);

    /* No ownership */
    set_channel_ownership(channel, 0);

    /* Cannot schedule it again */
    sdma_channel_set_priority(channel, 0);

    /* Reset channel control block entry */
    memset(ccb_p, 0x00, sizeof (struct channel_control_block));
}

/* Write 32-bit words to SDMA core memory. Host endian->SDMA endian. */
void sdma_write_words(const unsigned long *buf, unsigned long start, int count)
{
    /* Setup buffer descriptor with channel 0 command */
    set_buffer_descriptor(&c0_buffer_desc.bd,
                          C0_SETDM, 
                          BD_DONE | BD_WRAP | BD_EXTD,
                          count,
                          (void *)addr_virt_to_phys((unsigned long)buf),
                          (void *)start);

    SDMA_HSTART = 1ul;
    sdma_channel_wait_nonblocking(0);
}

/* Read 32-bit words from SDMA core memory. SDMA endian->host endian. */
void sdma_read_words(unsigned long *buf, unsigned long start, int count)
{
    /* Setup buffer descriptor with channel 0 command */
    set_buffer_descriptor(&c0_buffer_desc.bd,
                          C0_GETDM, 
                          BD_DONE | BD_WRAP | BD_EXTD,
                          count,
                          (void *)addr_virt_to_phys((unsigned long)buf),
                          (void *)start);

    SDMA_HSTART = 1ul;
    sdma_channel_wait_nonblocking(0);
}
