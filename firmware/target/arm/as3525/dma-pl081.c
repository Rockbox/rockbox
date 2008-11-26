/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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

#include <stdbool.h>
#include "as3525.h"
#include "pl081.h"
#include "dma-target.h"
#include "panic.h"
#include "kernel.h"

static struct wakeup transfer_completion_signal[2]; /* 2 channels */

inline void dma_wait_transfer(int channel)
{
    wakeup_wait(&transfer_completion_signal[channel], TIMEOUT_BLOCK);
}

void dma_init(void)
{
    /* Enable DMA controller */
    CGU_PERI |= CGU_DMA_CLOCK_ENABLE;
    DMAC_CONFIGURATION |= (1<<0);   /* TODO: disable controller when not used */
    DMAC_SYNC = 0;
    VIC_INT_ENABLE |= INTERRUPT_DMAC;

    wakeup_init(&transfer_completion_signal[0]);
    wakeup_init(&transfer_completion_signal[1]);
}

void dma_enable_channel(int channel, void *src, void *dst, int peri,
                        int flow_controller, bool src_inc, bool dst_inc,
                        size_t size, int nwords)
{
    int control = 0;

    DMAC_CH_SRC_ADDR(channel) = (int)src;
    DMAC_CH_DST_ADDR(channel) = (int)dst;

    DMAC_CH_LLI(channel) = 0;    /* we use contigous memory, so don't use the LLI */

    /* specify address increment */
    if(src_inc)
        control |= (1<<26);

    if(dst_inc)
        control |= (1<<27);

    /* OF use transfers of 4 * 32 bits words on memory, i2sin, i2sout */
    /* OF use transfers of 8 * 32 bits words on SD */

    control |= (2<<21) | (2<<18);  /* dst/src width = word, 32bit */
    control |= (nwords<<15) | (nwords<<12);  /* dst/src size  */
    control |= (size & 0x7ff);     /* transfer size */

    control |= (1<<31); /* current LLI is expected to trigger terminal count interrupt */

    DMAC_CH_CONTROL(channel) = control;

    /* only needed if DMAC and Peripheral do not run at the same clock speed */
    DMAC_SYNC |= (1<<peri);

    /* we set the same peripheral as source and destination because we always
     * use memory-to-peripheral or peripheral-to-memory transfers */
    DMAC_CH_CONFIGURATION(channel) =
        (flow_controller<<11)   /* flow controller is peripheral */
        | (1<<15)               /* terminal count interrupt mask */
        | (1<<14)               /* interrupt error mask */
        | (peri<<6)             /* dst peripheral */
        | (peri<<1)             /* src peripheral */
        | (1<<0)                /* enable channel */
        ;
}

/* isr */
void INT_DMAC(void)
{
    int channel = (DMAC_INT_STATUS & (1<<0)) ? 0 : 1;

    if(DMAC_INT_ERROR_STATUS & (1<<channel))
        panicf("DMA error, channel %d", channel);

    DMAC_INT_TC_CLEAR |= (1<<channel); /* clear terminal count interrupt */

    wakeup_signal(&transfer_completion_signal[channel]);
}
