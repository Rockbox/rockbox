/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 and 2009 by Karl Kurbjun
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

void pcm_postinit(void)
{
	audiohw_postinit();
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    (void) count;
    return 0;
}

void pcm_play_dma_init(void)
{
    IO_INTC_IRQ0 = 1 << 11;
    IO_INTC_EINT0 |= 1 << 11;
    
    IO_DSPC_HPIB_CONTROL = 1 << 10 | 1 << 9 | 1 << 8 | 1 << 7 | 1 << 3 | 1 << 0;
    
    dsp_reset();
    dsp_load(dsp_image);
}

void pcm_dma_apply_settings(void)
{
	audiohw_set_frequency(pcm_fsel);
}

void pcm_play_dma_start(const void *addr, size_t size)
{
	unsigned long sdem_addr=(unsigned long)addr - CONFIG_SDRAM_START;
    /* Initialize codec. */
    DSP_(_sdem_addrl) = sdem_addr & 0xffff;
    DSP_(_sdem_addrh) = sdem_addr >> 16;
    DSP_(_sdem_dsp_size) = size;
    dsp_wake();
}

void pcm_play_dma_stop(void)
{

}

void pcm_play_lock(void)
{

}

void pcm_play_unlock(void)
{

}

void pcm_play_dma_pause(bool pause)
{
    (void) pause;
}

size_t pcm_get_bytes_waiting(void)
{
    return DSP_(_sdem_dsp_size)-DSP_(_sdem_level);
}

void DSPHINT(void)
{
	static unsigned char *start;
	static size_t         size;
	register pcm_more_callback_type get_more;   /* No stack for this */
	
    unsigned int i;
    char buffer[80];

    IO_INTC_IRQ0 = 1 << 11;
    
    switch (dsp_message.msg) 
    {
    case MSG_DEBUGF:
        /* DSP stores one character per word. */
        for (i = 0; i < sizeof(buffer); i++) 
        {
            buffer[i] = dsp_message.payload.debugf.buffer[i];
        }
        
        /* Release shared area to DSP. */
        dsp_wake();
        
        DEBUGF("DSP: %s", buffer);
        break;
        
    case MSG_REFILL:
		/* Buffer empty.  Try to get more. */
		get_more = pcm_callback_for_more;
		size = 0;

		if (get_more == NULL || (get_more(&start, &size), size == 0))
		{
		    /* Callback missing or no more DMA to do */
		    pcm_play_dma_stop();
		    pcm_play_dma_stopped_callback();
		}
		else
		{
			unsigned long sdem_addr=(unsigned long)start - CONFIG_SDRAM_START;
		  	/* Flush any pending cache writes */
		    clean_dcache_range(start, size);

		    /* set the new DMA values */
		    DSP_(_sdem_addrl) = sdem_addr & 0xffff;
			DSP_(_sdem_addrh) = sdem_addr >> 16;
	   		DSP_(_sdem_dsp_size) = size;
	   		
	    	DEBUGF("pcm_sdram at 0x%08lx, sdem_addr 0x%08lx",
				(unsigned long)start, (unsigned long)sdem_addr);
		}
		break;
    default:
	    DEBUGF("DSP: unknown msg 0x%04x", dsp_message.msg);
	    break;
    }
    
    /* Re-Activate the channel */
    dsp_wake();
    
    DEBUGF("DSP: %s", buffer);
}
    
