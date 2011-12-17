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
#include "ccm-imx31.h"
#include "avic-imx31.h"
#include "sdma_struct.h"
#include "sdma-imx31.h"
#include "sdma_script_code.h"
#include "mmu-imx31.h"

/* Most of the code in here is based upon the Linux BSP provided by Freescale
 * Copyright 2004-2008 Freescale Semiconductor, Inc. All Rights Reserved. */

/* Cut down to bare bones essentials */

/* Mask of channels with callback enabled */
static unsigned long sdma_enabled_ints = 0;
/* One channel control block per channel in physically mapped device RAM */
static struct channel_control_block ccb_array[CH_NUM] NOCACHEBSS_ATTR;
/* Channel 0 (command channel) data */
static struct buffer_descriptor_extd c0_buffer_desc NOCACHEBSS_ATTR;

/* All SDMA channel interrupts are handled here.
 * Dispatches lower channel numbers first (prioritized by SDMA API callers
 * who specify the desired channel number).
 */
static void __attribute__((interrupt("IRQ"))) SDMA_HANDLER(void)
{
    unsigned long pending = SDMA_INTR;

    SDMA_INTR = pending;          /* Ack all ints */
    pending &= sdma_enabled_ints; /* Only dispatch ints with callback */

    while (pending)
    {
        unsigned int bit = pending & -pending; /* Isolate bottom bit */
        pending &= ~bit;          /* Clear it */

        /* Call callback (required if using an interrupt). bit number = channel */
        ccb_array[31 - __builtin_clz(bit)].channel_desc->callback();
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
            res = AP_2_AP_ADDR;
            break;
        }
        break;

#if 0 /* Not using this */
    case SDMA_PER_DSP:
        switch (transfer_type)
        {
        case SDMA_TRAN_EMI_2_DSP:
            res = AP_2_BP_ADDR;
            break;
        case SDMA_TRAN_DSP_2_EMI:
            res = BP_2_AP_ADDR;
            break;
        case SDMA_TRAN_DSP_2_EMI_LOOP:
            res = LOOPBACK_ON_DSP_SIDE_ADDR;
            break;
        case SDMA_TRAN_EMI_2_DSP_LOOP:
            res = MCU_INTERRUPT_ONLY_ADDR;
            break;
        }
        break;
#endif

#if 0 /* Not using this */
    case SDMA_PER_FIRI:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = FIRI_2_PER_ADDR;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = FIRI_2_MCU_ADDR;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = PER_2_FIRI_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_FIRI_ADDR;
            break;
        }
        break;
#endif

#if 0 /* Not using this */
    case SDMA_PER_UART:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = UART_2_PER_ADDR;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = UART_2_MCU_ADDR;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = PER_2_APP_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_APP_ADDR;
            break;
        }
        break;
#endif

#if 0 /* Not using this */
    case SDMA_PER_UART_SHP:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = UARTSH_2_PER_ADDR;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = UARTSH_2_MCU_ADDR;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = PER_2_SHP_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_SHP_ADDR;
            break;
        }
        break;
#endif

    case SDMA_PER_ATA:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_EMI:
            res = ATA_2_MCU_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_ATA_ADDR;
            break;
        }
        break;

    case SDMA_PER_CSPI:
    case SDMA_PER_EXT:
    case SDMA_PER_SSI:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_INT:
            res = APP_2_PER_ADDR;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = APP_2_MCU_ADDR;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = PER_2_APP_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_APP_ADDR;
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
            res = SHP_2_PER_ADDR;
            break;
        case SDMA_TRAN_PER_2_EMI:
            res = SHP_2_MCU_ADDR;
            break;
        case SDMA_TRAN_INT_2_PER:
            res = PER_2_SHP_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_SHP_ADDR;
            break;
        }
        break;

    case SDMA_PER_MSHC:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_EMI:
            res = MSHC_2_MCU_ADDR;
            break;
        case SDMA_TRAN_EMI_2_PER:
            res = MCU_2_MSHC_ADDR;
            break;
        }
        break;

    case SDMA_PER_CCM:
        switch (transfer_type)
        {
        case SDMA_TRAN_PER_2_EMI:
            res = DPTC_DVFS_ADDR;
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
    bitmod32(&SDMA_DSPOVR, (config & CH_OWNSHP_DSP) ? 0 : bit, bit);
#endif
    /* Event */
    bitmod32(&SDMA_EVTOVR, (config & CH_OWNSHP_EVT) ? 0 : bit, bit);
    /* MCU side */
    bitmod32(&SDMA_HOSTOVR, (config & CH_OWNSHP_MCU) ? 0 : bit, bit);
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
    commit_dcache_range(&context_buffer, sizeof (context_buffer));
    sdma_write_words((unsigned long *)&context_buffer,
                     CHANNEL_CONTEXT_ADDR(channel),
                     sizeof (context_buffer)/4);

    ccb_p->status.error = 0; /* Clear channel-wide error flag */

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
        bitset32(&SDMA_CHNENBL(cd_p->event_id1), 1ul << channel);

        if (cd_p->per_type == SDMA_PER_ATA)
        {
            /* ATA has two */
            bitset32(&SDMA_CHNENBL(cd_p->event_id2), 1ul << channel);
        }
    }

    cd_p->is_setup = 1;

    return true;
}

/** Public routines **/
void INIT_ATTR sdma_init(void)
{
    int i;
    unsigned long acr;

    ccm_module_clock_gating(CG_SDMA, CGM_ON_RUN_WAIT);

    /* Reset the controller */
    SDMA_RESET |= SDMA_RESET_RESET;
    while (SDMA_RESET & SDMA_RESET_RESET);

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

    avic_enable_int(INT_SDMA, INT_TYPE_IRQ, INT_PRIO_SDMA, SDMA_HANDLER);

    /* SDMA core must run at the proper frequency based upon the AHB/IPG
     * ratio */
    acr = (ccm_get_ahb_clk() / ccm_get_ipg_clk()) < 2 ? SDMA_CONFIG_ACR : 0;

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
                          RAM_CODE_SIZE,
                          (void *)addr_virt_to_phys(MCU_START_ADDR),
                          (void *)RAM_CODE_START_ADDR);

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
        bitset32(&sdma_enabled_ints, chmsk);

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
        bitset32(&sdma_enabled_ints, 1ul << channel);

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
    bitclr32(&sdma_enabled_ints, 1ul << channel);

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

/* Check channel-wide error flag */
bool sdma_channel_is_error(unsigned int channel)
{
    return channel < CH_NUM && ccb_array[channel].status.error;
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
