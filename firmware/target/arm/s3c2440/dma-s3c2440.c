/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright  2009 by Bob Cousins
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
#include "config.h"
#include "panic.h"
#include "system.h"
#include "mmu-arm.h"
#include "s3c2440.h"
#include "dma-target.h"
#include "system-target.h"

#define NUM_CHANNELS 4

static int dma_used = 0;

/* Status flags */
#define STATUS_CHANNEL_ACTIVE (1<<0)

static struct dma_channel_state 
{
    volatile unsigned status;
    void (*callback)(void);
} dma_state [NUM_CHANNELS];

struct dma_channel_regs 
{
    volatile unsigned long disrc;
    volatile unsigned long disrcc;
    volatile unsigned long didst;
    volatile unsigned long didstc;
    volatile unsigned long dcon;
    volatile unsigned long dstat;
    volatile unsigned long dcsrc;
    volatile unsigned long dcdst;
    volatile unsigned long dmasktrig;
    volatile unsigned long reserved [7];  /* pad to 0x40 bytes */ 
};


struct dma_channel_regs *dma_regs [4] =
    {
    (struct dma_channel_regs *) &DISRC0,
    (struct dma_channel_regs *) &DISRC1,
    (struct dma_channel_regs *) &DISRC2,
    (struct dma_channel_regs *) &DISRC3
    } 
;


void dma_init(void)
{
    /* TODO */

    /* Enable interupt on DMA Finish */
    /* Clear pending source */
    SRCPND = DMA0_MASK | DMA1_MASK | DMA2_MASK | DMA3_MASK;
    INTPND = DMA0_MASK | DMA1_MASK | DMA2_MASK | DMA3_MASK;
    
    /* Enable interrupt in controller */
    bitclr32(&INTMOD, DMA0_MASK | DMA1_MASK | DMA2_MASK | DMA3_MASK);
    bitclr32(&INTMSK, DMA0_MASK | DMA1_MASK | DMA2_MASK | DMA3_MASK);
}

void dma_retain(void)
{
    /* TODO */
    dma_used++;
    if(dma_used > 0)
    {
        /* Enable DMA controller, clock? */
    }
}

void dma_release(void)
{
    /* TODO */
    if (dma_used > 0)
        dma_used--;
    if(dma_used == 0)
    {
        /* Disable DMA */
    }
}


inline void dma_disable_channel(int channel)
{
    struct dma_channel_regs *regs = dma_regs [channel]; 
     
    /* disable the specified channel */
        
    /* Reset the channel */
    regs->dmasktrig |= DMASKTRIG_STOP;
    
    /* Wait for DMA controller to be ready */
    while(regs->dmasktrig & DMASKTRIG_ON)
        ;
    while(regs->dstat & DSTAT_STAT_BUSY)
        ;
}

void dma_enable_channel(int channel, struct dma_request *request)
{
    struct dma_channel_regs *regs = dma_regs [channel];
     
    /* TODO - transfer sizes (assumes word) */
    
    if (DMA_GET_SRC(request->source_map, channel) == DMA_INVALID)
        panicf ("DMA: invalid channel");
    
    /* setup a transfer on specified channel */
    dma_disable_channel (channel);
    
    if((unsigned long)request->source_addr < UNCACHED_BASE_ADDR)
        regs->disrc = (unsigned long)request->source_addr + UNCACHED_BASE_ADDR;
    else
        regs->disrc = (unsigned long)request->source_addr;
    regs->disrcc = request->source_control;
    
    if((unsigned long)request->dest_addr < UNCACHED_BASE_ADDR)
        regs->didst = (unsigned long)request->dest_addr + UNCACHED_BASE_ADDR;
    else
        regs->didst = (unsigned long)request->dest_addr;
    regs->didstc = request->dest_control;
    
    regs->dcon = request->control | request->count | 
                 DMA_GET_SRC(request->source_map, channel) * DCON_HWSRCSEL;

    dma_state [channel].callback = request->callback;
            
    /* Activate the channel */
    commit_discard_dcache_range((void *)request->dest_addr, request->count * 4);
    
    dma_state [channel].status |= STATUS_CHANNEL_ACTIVE;
    regs->dmasktrig = DMASKTRIG_ON;

    if ((request->control & DCON_HW_SEL) == 0)
    {
        /* Start DMA */
        regs->dmasktrig |= DMASKTRIG_SW_TRIG;
    }
        
}

/* ISRs */
static inline void generic_isr (unsigned channel)
{
    if (dma_state [channel].status | STATUS_CHANNEL_ACTIVE)
    {
        if (dma_state [channel].callback)
            /* call callback for relevant channel */
            dma_state [channel].callback();
    
        dma_state [channel].status &= ~STATUS_CHANNEL_ACTIVE;
    }
}

void DMA0(void)
{
    generic_isr (0);
    /* Ack the interrupt */
    SRCPND = DMA0_MASK;
    INTPND = DMA0_MASK;
}

void DMA1(void)
{
    generic_isr (1);
    /* Ack the interrupt */
    SRCPND = DMA1_MASK;
    INTPND = DMA1_MASK;
}

void DMA2(void)
{
    generic_isr (2);
    /* Ack the interrupt */
    SRCPND = DMA2_MASK;
    INTPND = DMA2_MASK;
}

void DMA3(void)
{
    generic_isr (3);
    /* Ack the interrupt */
    SRCPND = DMA3_MASK;
    INTPND = DMA3_MASK;
}
