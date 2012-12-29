/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "dcp-imx233.h"
#include "string.h"
#include "kernel-imx233.h"

/* The hardware uses 160 bytes of storage to enable context switching */
static uint8_t dcp_context[160] NOCACHEBSS_ATTR;
/* Channel arbiter */
static struct channel_arbiter_t channel_arbiter;
/* Channel packets */
static struct imx233_dcp_packet_t channel_packet[HW_DCP_NUM_CHANNELS];
/* completion semaphore */
static struct semaphore channel_sema[HW_DCP_NUM_CHANNELS];

void INT_DCP(void)
{
    /* clear interrupt and wakeup completion handler */
    for(int i = 0; i < HW_DCP_NUM_CHANNELS; i++)
    {
        if(HW_DCP_STAT & HW_DCP_STAT__IRQ(i))
        {
            __REG_CLR(HW_DCP_STAT) = HW_DCP_STAT__IRQ(i);
            semaphore_release(&channel_sema[i]);
        }
    }
}

void imx233_dcp_init(void)
{
    /* Reset block */
    imx233_reset_block(&HW_DCP_CTRL);
    /* Setup contexte pointer */
    HW_DCP_CONTEXT = (uint32_t)PHYSICAL_ADDR(&dcp_context);
    /* Enable context switching and caching */
    __REG_SET(HW_DCP_CTRL) = HW_DCP_CTRL__ENABLE_CONTEXT_CACHING |
        HW_DCP_CTRL__ENABLE_CONTEXT_SWITCHING;
    /* Check that there are sufficiently many channels */
    if(__XTRACT(HW_DCP_CAPABILITY0, NUM_CHANNELS) != HW_DCP_NUM_CHANNELS)
        panicf("DCP has %lu channels but was configured to use %d !",
               __XTRACT(HW_DCP_CAPABILITY0, NUM_CHANNELS), HW_DCP_NUM_CHANNELS);
    /* Setup channel arbiter to use */
    arbiter_init(&channel_arbiter, HW_DCP_NUM_CHANNELS);
    /* Merge channel0 interrupt */
    __REG_SET(HW_DCP_CHANNELCTRL) = HW_DCP_CHANNELCTRL__CH0_IRQ_MERGED;
    /* setup semaphores */
    for(int i = 0; i< HW_DCP_NUM_CHANNELS; i++)
        semaphore_init(&channel_sema[i], 1, 0);
}

// return OBJ_WAIT_TIMEOUT on failure
int imx233_dcp_acquire_channel(int timeout)
{
    return arbiter_acquire(&channel_arbiter, timeout);
}

void imx233_dcp_release_channel(int chan)
{
    arbiter_release(&channel_arbiter, chan);
}

// doesn't check that channel is in use!
void imx233_dcp_reserve_channel(int channel)
{
    arbiter_reserve(&channel_arbiter, channel);
}

static enum imx233_dcp_error_t get_error_status(int ch)
{
    uint32_t stat = channel_packet[ch].status;
    if(stat & HW_DCP_STATUS__ERROR_SETUP)
        return DCP_ERROR_SETUP;
    if(stat & HW_DCP_STATUS__ERROR_PACKET)
        return DCP_ERROR_PACKET;
    if(stat & HW_DCP_STATUS__ERROR_SRC)
        return DCP_ERROR_SRC;
    if(stat & HW_DCP_STATUS__ERROR_DST)
        return DCP_ERROR_DST;
    switch(__XTRACT_EX(stat, HW_DCP_STATUS__ERROR_CODE))
    {
        case 0: return DCP_SUCCESS;
        case 1: return DCP_ERROR_CHAIN_IS_0;
        case 2: return DCP_ERROR_NO_CHAIN;
        case 3: return DCP_ERROR_CONTEXT;
        case 4: return DCP_ERROR_PAYLOAD;
        case 5: return DCP_ERROR_MODE;
        default: return DCP_ERROR;
    }
}

static enum imx233_dcp_error_t imx233_dcp_job(int ch)
{
    /* if IRQs are not enabled, don't enable channel interrupt and do some polling */
    bool irq_enabled = irq_enabled();
    /* enable channel, clear interrupt, enable interrupt */
    imx233_icoll_enable_interrupt(INT_SRC_DCP, true);
    if(irq_enabled)
        __REG_SET(HW_DCP_CTRL) = HW_DCP_CTRL__CHANNEL_INTERRUPT_ENABLE(ch);
    __REG_CLR(HW_DCP_STAT) = HW_DCP_STAT__IRQ(ch);
    __REG_SET(HW_DCP_CHANNELCTRL) = HW_DCP_CHANNELCTRL__ENABLE_CHANNEL(ch);

    /* write back packet */
    commit_discard_dcache_range(&channel_packet[ch], sizeof(struct imx233_dcp_packet_t));
    /* write 1 to semaphore to run job */
    HW_DCP_CHxCMDPTR(ch) = (uint32_t)PHYSICAL_ADDR(&channel_packet[ch]);
    HW_DCP_CHxSEMA(ch) = 1;
    /* wait completion */
    if(irq_enabled)
        semaphore_wait(&channel_sema[ch], TIMEOUT_BLOCK);
    else
        while(__XTRACT_EX(HW_DCP_CHxSEMA(ch), HW_DCP_CHxSEMA__VALUE))
            udelay(10);
    /* disable channel and interrupt */
    __REG_CLR(HW_DCP_CTRL) = HW_DCP_CTRL__CHANNEL_INTERRUPT_ENABLE(ch);
    __REG_CLR(HW_DCP_CHANNELCTRL) = HW_DCP_CHANNELCTRL__ENABLE_CHANNEL(ch);
    /* read status */
    return get_error_status(ch);
}


enum imx233_dcp_error_t imx233_dcp_memcpy_ex(int ch, bool fill, const void *src, void *dst, size_t len)
{
    /* prepare packet */
    channel_packet[ch].next = 0;
    channel_packet[ch].ctrl0 = HW_DCP_CTRL0__INTERRUPT_ENABLE |
        HW_DCP_CTRL0__ENABLE_MEMCOPY | HW_DCP_CTRL0__DECR_SEMAPHORE |
        (fill ? HW_DCP_CTRL0__CONSTANT_FILL : 0);
    channel_packet[ch].ctrl1 = 0;
    channel_packet[ch].src = (uint32_t)(fill ? src : PHYSICAL_ADDR(src));
    channel_packet[ch].dst = (uint32_t)PHYSICAL_ADDR(dst);
    channel_packet[ch].size = len;
    channel_packet[ch].payload = 0;
    channel_packet[ch].status = 0;

    /* write-back src if not filling, discard dst */
    if(!fill)
        commit_discard_dcache_range(src, len);
    discard_dcache_range(dst, len);

    /* do the job */
    return imx233_dcp_job(ch);
}

enum imx233_dcp_error_t imx233_dcp_memcpy(bool fill, const void *src, void *dst, size_t len, int tmo)
{
    int chan = imx233_dcp_acquire_channel(tmo);
    if(chan == OBJ_WAIT_TIMEDOUT)
        return DCP_TIMEOUT;
    enum imx233_dcp_error_t err = imx233_dcp_memcpy_ex(chan, fill, src, dst, len);
    imx233_dcp_release_channel(chan);
    return err;
}

enum imx233_dcp_error_t imx233_dcp_blit_ex(int ch, bool fill, const void *src, size_t w, size_t h, void *dst, size_t out_w)
{
    /* prepare packet */
    channel_packet[ch].next = 0;
    channel_packet[ch].ctrl0 = HW_DCP_CTRL0__INTERRUPT_ENABLE |
    HW_DCP_CTRL0__ENABLE_MEMCOPY | HW_DCP_CTRL0__DECR_SEMAPHORE |
    HW_DCP_CTRL0__ENABLE_BLIT |
    (fill ? HW_DCP_CTRL0__CONSTANT_FILL : 0);
    channel_packet[ch].ctrl1 = out_w;
    channel_packet[ch].src = (uint32_t)(fill ? src : PHYSICAL_ADDR(src));
    channel_packet[ch].dst = (uint32_t)PHYSICAL_ADDR(dst);
    channel_packet[ch].size = w | h << HW_DCP_SIZE__NUMBER_LINES_BP;
    channel_packet[ch].payload = 0;
    channel_packet[ch].status = 0;
    
    /* we have a problem here to discard the output buffer since it's not contiguous
     * so only commit the source */
    if(!fill)
        commit_discard_dcache_range(src, w * h);
    /* do the job */
    return imx233_dcp_job(ch);
}

enum imx233_dcp_error_t imx233_dcp_blit(bool fill, const void *src, size_t w, size_t h, void *dst, size_t out_w, int tmo)
{
    int chan = imx233_dcp_acquire_channel(tmo);
    if(chan == OBJ_WAIT_TIMEDOUT)
        return DCP_TIMEOUT;
    enum imx233_dcp_error_t err = imx233_dcp_blit_ex(chan, fill, src, w, h, dst, out_w);
    imx233_dcp_release_channel(chan);
    return err;
}

struct imx233_dcp_info_t imx233_dcp_get_info(unsigned flags)
{
    struct imx233_dcp_info_t info;
    memset(&info, 0, sizeof(info));
    if(flags & DCP_INFO_CAPABILITIES)
    {
        info.has_crypto = HW_DCP_CTRL & HW_DCP_CTRL__PRESENT_CRYPTO;
        info.has_csc = HW_DCP_CTRL & HW_DCP_CTRL__PRESENT_CSC;
        info.num_keys = __XTRACT(HW_DCP_CAPABILITY0, NUM_KEYS);
        info.num_channels = __XTRACT(HW_DCP_CAPABILITY0, NUM_CHANNELS);
        info.ciphers = __XTRACT(HW_DCP_CAPABILITY1, CIPHER_ALGORITHMS);
        info.hashs = __XTRACT(HW_DCP_CAPABILITY1, HASH_ALGORITHMS);
    }
    if(flags & DCP_INFO_GLOBAL_STATE)
    {
        info.otp_key_ready = HW_DCP_STAT & HW_DCP_STAT__OTP_KEY_READY;
        info.context_switching = HW_DCP_CTRL & HW_DCP_CTRL__ENABLE_CONTEXT_SWITCHING;
        info.context_caching = HW_DCP_CTRL & HW_DCP_CTRL__ENABLE_CONTEXT_CACHING;
        info.gather_writes = HW_DCP_CTRL & HW_DCP_CTRL__GATHER_RESIDUAL_WRITES;
        info.ch0_merged = HW_DCP_CHANNELCTRL & HW_DCP_CHANNELCTRL__CH0_IRQ_MERGED;
    }
    if(flags & DCP_INFO_CHANNELS)
    {
        for(int i = 0; i < HW_DCP_NUM_CHANNELS; i++)
        {
            info.channel[i].irq_en = HW_DCP_CTRL & HW_DCP_CTRL__CHANNEL_INTERRUPT_ENABLE(i);
            info.channel[i].irq = HW_DCP_STAT & HW_DCP_STAT__IRQ(i);
            info.channel[i].ready = HW_DCP_STAT & HW_DCP_STAT__READY_CHANNELS(i);
            info.channel[i].high_priority = HW_DCP_CHANNELCTRL & HW_DCP_CHANNELCTRL__HIGH_PRIORITY_CHANNEL(i);
            info.channel[i].enable = HW_DCP_CHANNELCTRL & HW_DCP_CHANNELCTRL__ENABLE_CHANNEL(i);
            info.channel[i].sema = __XTRACT_EX(HW_DCP_CHxSEMA(i), HW_DCP_CHxSEMA__VALUE);
            info.channel[i].cmdptr = HW_DCP_CHxCMDPTR(i);
            info.channel[i].acquired = arbiter_acquired(&channel_arbiter, i);
        }
    }
    if(flags & DCP_INFO_CSC)
    {
        info.csc.irq_en = HW_DCP_CTRL & HW_DCP_CTRL__CSC_INTERRUPT_ENABLE;
        info.csc.irq = HW_DCP_STAT & HW_DCP_STAT__CSCIRQ;
        info.csc.priority = __XTRACT(HW_DCP_CHANNELCTRL, CSC_PRIORITY);
        info.csc.enable = HW_DCP_CSCCTRL0 & HW_DCP_CSCCTRL0__ENABLE;
    }
    return info;
}