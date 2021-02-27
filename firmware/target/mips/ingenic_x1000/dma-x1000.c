/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "system.h"
#include "dma-x1000.h"
#include "irq-x1000.h"
#include "x1000/cpm.h"
#include "panic.h"

static dma_cb_func dma_callbacks[DMA_NUM_USED_CHANNELS];

static void dma_no_cb(int event)
{
    (void)event;
    panicf("Unhandled DMA channel interrupt");
}

void dma_init(void)
{
    for(int i = 0; i < DMA_NUM_USED_CHANNELS; ++i)
        dma_callbacks[i] = dma_no_cb;

    jz_writef(CPM_CLKGR, PDMA(0));
    jz_writef(DMA_CTRL, ENABLE(1), HALT(0), AR(0));
    jz_writef(DMA_CTRL, FMSC(1), FSSI(1), FTSSI(1), FUART(1), FAIC(1));
    system_enable_irq(IRQ_PDMA);
    system_enable_irq(IRQ_PDMAD);
}

void dma_set_callback(int chn, dma_cb_func cb)
{
    dma_callbacks[chn] = cb != NULL ? cb : dma_no_cb;
}

void PDMA(void)
{
    /* This is called when the last descriptor completes, or if the
     * channel hits an error.
     */
    unsigned pending = REG_DMA_IRQP;
    for(int i = 0; i < DMA_NUM_USED_CHANNELS; ++i) {
        if((pending & (1 << i)) == 0)
            continue;

        int evt;
        if(REG_DMA_CHN_CS(i) & jz_orm(DMA_CHN_CS, AR, HLT))
            evt = DMA_EVENT_ERROR;
        else
            evt = DMA_EVENT_COMPLETE;

        REG_DMA_CHN_CS(i) = 0;
        dma_callbacks[i](evt);
    }

    /* Clear any errors and clear interrupts */
    jz_writef(DMA_CTRL, HALT(0), AR(0));
    REG_DMA_IRQP = 0;
}

void PDMAD(void)
{
    /* Called when TIE is set on a non-final descriptor */
    unsigned pending = REG_DMA_DIP;
    for(int i = 0; i < DMA_NUM_USED_CHANNELS; ++i) {
        if((pending & (1 << i)) == 0)
            continue;

        dma_callbacks[i](DMA_EVENT_INTERRUPT);
    }

    /* This does not operate like other clear registers */
    REG_DMA_DIC &= ~pending;
}
