/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include "dma-imx233.h"

void imx233_dma_init(void)
{
    /* Enable APHB and APBX */
    __REG_CLR(HW_APBH_CTRL0) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
    __REG_CLR(HW_APBX_CTRL0) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
}

void imx233_dma_reset_channel(unsigned chan)
{
    if(APB_IS_APBX_CHANNEL(chan))
        __REG_SET(HW_APBX_CHANNEL_CTRL) =
            HW_APBX_CHANNEL_CTRL__RESET_CHANNEL(APB_GET_DMA_CHANNEL(chan));
    else
        __REG_SET(HW_APBH_CTRL0) =
            HW_APBH_CTRL0__RESET_CHANNEL(APB_GET_DMA_CHANNEL(chan));
}

void imx233_dma_clkgate_channel(unsigned chan, bool enable_clock)
{
    if(APB_IS_APBX_CHANNEL(chan))
        return;
    if(enable_clock)
        __REG_CLR(HW_APBH_CTRL0) =
            HW_APBH_CTRL0__CLKGATE_CHANNEL(APB_GET_DMA_CHANNEL(chan));
    else
        __REG_SET(HW_APBH_CTRL0) =
            HW_APBH_CTRL0__CLKGATE_CHANNEL(APB_GET_DMA_CHANNEL(chan));
}

void imx233_dma_enable_channel_interrupt(unsigned chan, bool enable)
{
    volatile uint32_t *ptr;
    uint32_t bm;
    if(APB_IS_APBX_CHANNEL(chan))
    {
        ptr = &HW_APBX_CTRL1;
        bm = HW_APBX_CTRL1__CHx_CMDCMPLT_IRQ_EN(APB_GET_DMA_CHANNEL(chan));
    }
    else
    {
        ptr = &HW_APBH_CTRL1;;
        bm = HW_APBH_CTRL1__CHx_CMDCMPLT_IRQ_EN(APB_GET_DMA_CHANNEL(chan));
    }

    if(enable)
    {
        __REG_SET(*ptr) = bm;
        imx233_dma_clear_channel_interrupt(chan);
    }
    else
        __REG_CLR(*ptr) = bm;
}

void imx233_dma_clear_channel_interrupt(unsigned chan)
{
    if(APB_IS_APBX_CHANNEL(chan))
    {
        __REG_CLR(HW_APBX_CTRL1) =
            HW_APBX_CTRL1__CHx_CMDCMPLT_IRQ(APB_GET_DMA_CHANNEL(chan));
        __REG_CLR(HW_APBX_CTRL2) =
            HW_APBX_CTRL2__CHx_ERROR_IRQ(APB_GET_DMA_CHANNEL(chan));
    }
    else
    {
        __REG_CLR(HW_APBH_CTRL1) =
            HW_APBH_CTRL1__CHx_CMDCMPLT_IRQ(APB_GET_DMA_CHANNEL(chan));
        __REG_CLR(HW_APBH_CTRL2) =
            HW_APBH_CTRL2__CHx_ERROR_IRQ(APB_GET_DMA_CHANNEL(chan));
    }
}

bool imx233_dma_is_channel_error_irq(unsigned chan)
{
    if(APB_IS_APBX_CHANNEL(chan))
        return !!(HW_APBX_CTRL2 &
            HW_APBX_CTRL2__CHx_ERROR_IRQ(APB_GET_DMA_CHANNEL(chan)));
    else
        return !!(HW_APBH_CTRL2 &
            HW_APBH_CTRL2__CHx_ERROR_IRQ(APB_GET_DMA_CHANNEL(chan)));
}

/* Commit and/or discard all DMA descriptors and buffers pointed by them,
 * handle circular lists. At the same time, convert virtual pointers to
 * real ones */
static void imx233_dma_commit_and_discard(struct apb_dma_command_t *cmd)
{
    /* We handle circular descriptors by using unused bits:
     * bits 8-11 are not used by the hardware so we first go through the whole
     * list and mark them all a special value at the same time we commit buffers
     * and then we go through the list another time to clear the mark and
     * commit the descriptors */
    struct apb_dma_command_t *cur = cmd;
    
    while((cur->cmd & HW_APB_CHx_CMD__UNUSED_BM) != HW_APB_CHx_CMD__UNUSED_MAGIC)
    {
        cur->cmd = (cur->cmd & ~HW_APB_CHx_CMD__UNUSED_BM) | HW_APB_CHx_CMD__UNUSED_MAGIC;
        int op = cur->cmd & HW_APB_CHx_CMD__COMMAND_BM;
        int sz = __XTRACT_EX(cur->cmd, HW_APB_CHx_CMD__XFER_COUNT);
        /* device > host: discard */
        if(op == HW_APB_CHx_CMD__COMMAND__WRITE)
            discard_dcache_range(cur->buffer, sz);
        /* host > device: commit and discard */
        else if(op == HW_APB_CHx_CMD__COMMAND__READ)
            commit_discard_dcache_range(cur->buffer, sz);
        /* Virtual to physical buffer pointer conversion */
        cur->buffer = PHYSICAL_ADDR(cur->buffer);
        /* chain ? */
        if(cur->cmd & HW_APB_CHx_CMD__CHAIN)
            cur = cur->next;
        else
            break;
    }

    cur = cmd;
    while((cur->cmd & HW_APB_CHx_CMD__UNUSED_BM) != 0)
    {
        cur->cmd = cur->cmd & ~HW_APB_CHx_CMD__UNUSED_BM;
        int sz = __XTRACT_EX(cur->cmd, HW_APB_CHx_CMD__CMDWORDS) * sizeof(uint32_t);
        /* commit descriptor and discard descriptor */
        /* chain ? */
        if(cur->cmd & HW_APB_CHx_CMD__CHAIN)
        {
            struct apb_dma_command_t *next = cur->next;
            cur->next = PHYSICAL_ADDR(cur->next);
            commit_dcache_range(cur, sizeof(struct apb_dma_command_t) + sz);
            cur = next;
        }
        else
        {
            commit_dcache_range(cur, sizeof(struct apb_dma_command_t) + sz);
            break;
        }
    }
}

void imx233_dma_start_command(unsigned chan, struct apb_dma_command_t *cmd)
{
    imx233_dma_commit_and_discard(cmd);
    if(APB_IS_APBX_CHANNEL(chan))
    {
        HW_APBX_CHx_NXTCMDAR(APB_GET_DMA_CHANNEL(chan)) = (uint32_t)PHYSICAL_ADDR(cmd);
        HW_APBX_CHx_SEMA(APB_GET_DMA_CHANNEL(chan)) = 1;
    }
    else
    {
        HW_APBH_CHx_NXTCMDAR(APB_GET_DMA_CHANNEL(chan)) = (uint32_t)PHYSICAL_ADDR(cmd);
        HW_APBH_CHx_SEMA(APB_GET_DMA_CHANNEL(chan)) = 1;
    }
}

void imx233_dma_wait_completion(unsigned chan)
{
    volatile uint32_t *sema;
    if(APB_IS_APBX_CHANNEL(chan))
        sema = &HW_APBX_CHx_SEMA(APB_GET_DMA_CHANNEL(chan));
    else
        sema = &HW_APBH_CHx_SEMA(APB_GET_DMA_CHANNEL(chan));

    while(*sema & HW_APB_CHx_SEMA__PHORE_BM)
        yield();
}
