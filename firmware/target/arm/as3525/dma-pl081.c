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
#include "config.h"

static int dma_used = 0;
static void (*dma_callback[2])(void); /* 2 channels */

void dma_retain(void)
{
    if(++dma_used == 1)
    {
        CGU_PERI |= CGU_DMA_CLOCK_ENABLE;
        DMAC_CONFIGURATION |= (1<<0);
    }
}

void dma_release(void)
{
    if(--dma_used == 0)
    {
        DMAC_CONFIGURATION &= ~(1<<0);
        CGU_PERI &= ~CGU_DMA_CLOCK_ENABLE;
    }
}

void dma_init(void)
{
#if CONFIG_CPU == AS3525
    DMAC_SYNC = 0xffff; /* disable synchronisation logic */
#endif
    VIC_INT_ENABLE = INTERRUPT_DMAC;
}

inline void dma_disable_channel(int channel)
{
    DMAC_CH_CONFIGURATION(channel) &= ~(1<<0);
}

void dma_enable_channel(int channel, void *src, void *dst, int peri,
                        int flow_controller, bool src_inc, bool dst_inc,
                        size_t size, int nwords, void (*callback)(void))
{
    dma_callback[channel] = callback;

    /* Clear any pending interrupts leftover from previous operation */
    DMAC_INT_TC_CLEAR  = (1<<channel);
    DMAC_INT_ERR_CLEAR = (1<<channel);

    DMAC_CH_SRC_ADDR(channel) = (int)src;
    DMAC_CH_DST_ADDR(channel) = (int)dst;

    /* When LLI is 0 channel is disabled upon transfer completion */
    DMAC_CH_LLI(channel) = 0;

    /*  Channel Control Register */
    DMAC_CH_CONTROL(channel) =
         ((1<<31)                 /* LLI triggers terminal count interrupt */
     /* | (1<<30) */              /* cacheable  = 1,  non = 0 */
     /* | (1<<29) */              /* bufferable = 1,  non = 0 */
     /* | (1<<28) */              /* privileged = 1, user = 0 */
        | (dst_inc? (1<<27): 0)   /* specify address increment */
        | (src_inc? (1<<26): 0)   /* specify address increment */
       /* [25:24] */              /* undefined  */
        | (2<<21)                 /* dst width = word, 32bit */
        | (2<<18)                 /* src width = word, 32bit */
       /* OF uses transfers of 4 * 32 bits words on memory, i2sin, i2sout */
       /* OF uses transfers of 8 * 32 bits words on SD */
        | (nwords<<15)            /* dst size  */
        | (nwords<<12)            /* src size  */
        | ((size & 0x7ff)<<0));   /* transfer size */

    /*  Channel Config Register  */
    DMAC_CH_CONFIGURATION(channel) =
       /* [31:19] */              /* Read undefined. Write as zero  */
       /* (0<<18) */              /* Halt Bit    */
       /* (0<<17) */              /* Active Bit  */
       /* (0<<16) */              /* Lock Bit    */
          (1<<15)                 /* terminal count interrupt mask */
        | (1<<14)                 /* interrupt error mask */
        | (flow_controller<<11)   /* flow controller is peripheral or SDMAC */
       /* we set the same peripheral as source and destination because we
        * always use memory-to-peripheral or peripheral-to-memory transfers */
        | (peri<<6)               /* dst peripheral */
        | (peri<<1)               /* src peripheral */
        | (1<<0);                 /* enable channel */
}

/* isr */
void INT_DMAC(void)
{
    unsigned int channel;

    /* SD channel is serviced first */
    for(channel = 0; channel < 2; channel++)
        if(DMAC_INT_STATUS & (1<<channel))
        {
            if(DMAC_INT_ERROR_STATUS & (1<<channel))
                panicf("DMA error, channel %d", channel);

            /* clear terminal count interrupt */
            DMAC_INT_TC_CLEAR = (1<<channel);

            if(dma_callback[channel])
                dma_callback[channel]();
        }
}
