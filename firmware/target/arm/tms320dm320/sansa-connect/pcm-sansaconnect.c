/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2011 by Tomasz Moń
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
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "file.h"
#include "dsp-target.h"
#include "dsp/ipc.h"
#include "mmu-arm.h"
#include "pcm-internal.h"
#include "dma-target.h"

/* This is global to save some latency when pcm_play_dma_get_peak_buffer is 
 *  called.
 */
static const void *start;
static int dma_channel;

/* Return the current location in the SDRAM to SARAM transfer along with the
 *  number of bytes read in the current buffer (count).  There is latency with
 *  this method equivalent to ~ the size of the SARAM buffer since there is
 *  another buffer between your ears and this calculation, but this works for
 *  key clicks and an approximate peak meter.
 */
const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    int oldlevel = disable_irq_save();
    unsigned long addr = (unsigned long)*(volatile const void **)&start;
    unsigned long size = DSP_(_sdem_level);
    restore_irq(oldlevel);

    *frames_rem = (size & 0xFFFFF) / 4;
    return (void *)((addr + size + 2) & ~3);
}

void pcm_dma_init(const struct pcm_hw_settings *settings)
{
    /* GIO16 is DSP/AIC3X CLK */
    IO_GIO_FSEL0 &= 0x3FFF;
    IO_CLK_OSEL = (IO_CLK_OSEL & 0xFFF0) | 4; /* PLLIN clock */
    IO_CLK_O0DIV = 7;
    IO_GIO_DIR1 &= ~(1 << 0); /* GIO16 - output */
    IO_GIO_FSEL0 |= 0xC000; /* GIO16 - CLKOUT0 */

    audiohw_init();
    audiohw_set_frequency(HW_FREQ_DEFAULT);

    IO_INTC_IRQ0 = INTR_IRQ0_IMGBUF;
    bitset16(&IO_INTC_EINT0, INTR_EINT0_IMGBUF);
    
    /* Set this as a FIQ */
    bitset16(&IO_INTC_FISEL0, INTR_EINT0_IMGBUF);
    
    /* Enable the HPIB clock */
    bitset16(&IO_CLK_MOD0, (CLK_MOD0_HPIB | CLK_MOD0_DSP));

    dma_channel = dma_request_channel(DMA_PERIPHERAL_DSP,
                                      DMA_MODE_1_BURST);

    IO_DSPC_HPIB_CONTROL = 1 << 10 | 1 << 9 | 1 << 8 | 1 << 7 | 1 << 3 | 1 << 0;
 
    dsp_reset();
    dsp_load(dsp_image);

    DSP_(_dma0_stopped)=1;
    dsp_wake();

    (void)settings;
}

void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    audiohw_set_frequency(settings->fsel);
}

/* Note that size is actually limited to the size of a short right now due to
 *  the implementation on the DSP side (and the way that we access it)
 */
void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    unsigned long sdem_addr=(unsigned long)addr - CONFIG_SDRAM_START;
    size_t size = frames*4;

    /* set the new DMA values */
    DSP_(_sdem_addrl) = sdem_addr & 0xffff;
    DSP_(_sdem_addrh) = sdem_addr >> 16;
    DSP_(_sdem_dsp_size) = size;

    /* Flush any pending cache writes */
    commit_dcache_range(start, size);

    if (DSP_(_dma0_stopped) == 1)
    {
        /* Initialize codec. */
        DSP_(_dma0_stopped)=0;
        dsp_wake();
    }
    else
    {
        /* Wakes it in DSPHINT() */
        DEBUGF("pcm_sdram at %08p, sdem_addr %08p", addr, sdem_addr);
    }
}

void pcm_play_dma_prepare(void)
{
    pcm_play_dma_stop();
}

void pcm_play_dma_stop(void)
{
    DSP_(_dma0_stopped)=1;
    start = NULL;
    dsp_wake();
}

void pcm_play_dma_lock(void)
{

}

void pcm_play_dma_unlock(void)
{

}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        DSP_(_dma0_stopped)=2;
        dsp_wake();
    }
    else
    {
        DSP_(_dma0_stopped)=0;
        dsp_wake();
    }
}

unsigned long pcm_play_dma_get_frames_waiting(void)
{
    return (DSP_(_sdem_dsp_size)-DSP_(_sdem_level)) / 4;
}

/* Only used when debugging */
static char buffer[80];

void DSPHINT(void) __attribute__ ((section(".icode")));
void DSPHINT(void)
{
    unsigned int i;
    unsigned long frames;

    IO_INTC_FIQ0 = INTR_IRQ0_IMGBUF;

    switch (dsp_message.msg) 
    {
    case MSG_DEBUGF:
        /* DSP stores one character per word. */
        for (i = 0; i < sizeof(buffer); i++) 
        {
            buffer[i] = dsp_message.payload.debugf.buffer[i];
        }

        DEBUGF("DSP: %s", buffer);
        break;
        
    case MSG_REFILL:
        /* Buffer empty. Inform and more may be sent. */
        pcm_play_dma_complete_callback(0);        
        break;
    default:
        DEBUGF("DSP: unknown msg 0x%04x", dsp_message.msg);
        break;
    }

    /* Re-Activate the channel */
    dsp_wake();
    
    DEBUGF("DSP: %s", buffer);
}
    
