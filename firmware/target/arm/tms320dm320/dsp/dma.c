/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Catalin Patulea
 * Copyright (C) 2008 by Maurus Cuelenaere
 * Copyright (C) 2009 by Karl Kurbjun
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

#include "registers.h"
#include "arm.h"
#include "dma.h"

/* This is placed at the right (aligned) address using linker.cmd. */
#pragma DATA_SECTION (data, ".dma")

#define DSP_BUFFER_SIZE PCM_SIZE/2

/* This is the buffer on the DSP side used for SARAM to McBSP (IIS) */
signed short data[DSP_BUFFER_SIZE];

/* These two describe the location of the buffer on the ARM (set in DSPHINT 
 *  and dspinit)
 */
volatile unsigned short sdem_addrh;
volatile unsigned short sdem_addrl;

/* This is the size of the ARM buffer (set in DSPHINT and dspinit) */
volatile unsigned short sdem_dsp_size;

/* These two variables keep track of the buffer level in the DSP, dsp_level,
 *  (SARAM to McBSP) and the level on the ARM buffer (sdem_level).
 */
unsigned short dsp_level=0;
unsigned short sdem_level=0;

/* This is used to keep track of the last SDRAM to SARAM transfer */
unsigned short last_size;

/* This tells us which half of the DSP buffer (data) is free */
unsigned short dma0_unlocked;

volatile unsigned short dma0_stopped=1;

short waiting=0;
/* rebuffer sets up the next SDRAM to SARAM transfer and tells the ARM when it
 * is done with a buffer.
 */
 
/* Notes: The upper limit on larger buffers is the size of a short.  If larger 
 *  buffer sizes are needed the code on the ARM side needs to be changed to 
 *  update a full long.
 */
void rebuffer(void)
{
	unsigned long sdem_addr;
	
	if(dma0_stopped==1) /* Stop */
	{
		DMPREC&=0xFFFE; /* Stop MCBSP DMA0 */
		audiohw_stop();
		DMSFC0 = 2 << 12 | 1 << 11; 
		sdem_level=0;
		return;
	}
	
   	if(dsp_level==DSP_BUFFER_SIZE || sdem_level==sdem_dsp_size)
   	{
   		if(dma0_stopped==2) /* Pause */
		{
			DMPREC&=0xFFFE; /* Stop MCBSP DMA0 */
			audiohw_stop();
			return;
		}
	}
	
	/* If the sdem_level is equal to the buffer size the ARM code gave
	 *  (sdem_dsp_size) then reset the size and ask the arm for another buffer
	 */
	if(sdem_level==sdem_dsp_size)
	{	
		sdem_level=0;
		
		/* Get a new buffer (location and size) from ARM */
		status.msg = MSG_REFILL;
		waiting=1;
		
		startack();
	}
	
	if(!waiting)
	{		
		/* Size is in bytes (but forced 32 bit transfers */
		if( (dsp_level + (sdem_dsp_size - sdem_level) ) > DSP_BUFFER_SIZE)
		{
			last_size = DSP_BUFFER_SIZE-dsp_level;
		}
		else
		{
			last_size = sdem_dsp_size-sdem_level;
		}
	
		/* DSP addresses are 16 bit (word) */
		DSP_ADDRL = (unsigned short)data + (dma0_unlocked >> 1) + (dsp_level>>1);
		DSP_ADDRH = 0;
	
		/* SDRAM addresses are 8 bit (byte)
		 * Warning: These addresses are forced to 32 bit alignment!
		 */
		sdem_addr = ((unsigned long)sdem_addrh << 16 | sdem_addrl) + sdem_level;
		SDEM_ADDRL = sdem_addr & 0xffff;
		SDEM_ADDRH = sdem_addr >> 16;
	
		/* Set the size of the SDRAM to SARAM transfer (demac transfer) */
		DMA_SIZE = last_size;

		DMA_CTRL = 0;

		/* These are just debug signals that are not used/needed right now */
		status.payload.refill._DMA_TRG = DMA_TRG;
		status.payload.refill._SDEM_ADDRH = SDEM_ADDRH;
		status.payload.refill._SDEM_ADDRL = SDEM_ADDRL;
		status.payload.refill._DSP_ADDRH = DSP_ADDRH;
		status.payload.refill._DSP_ADDRL = DSP_ADDRL;

		/* Start the demac transfer */
		DMA_TRG = 1;
	}
}

/* This interupt handler is for the SARAM (on DSP) to McBSP IIS DMA transfer.
 * It interupts at 1/2 empty and empty so that we can start filling a new buffer
 * from SDRAM when a half is free. dsp_level should always be full when this
 * interupt occurs except for the initial start.
 */
interrupt void handle_dma0(void) 
{
    /* Byte offset to half-buffer locked by DMA0.
            0 for top, PCM_SIZE/2 for bottom */
    unsigned short dma0_locked;
    
    IFR = 1 << 6;

    /* DMSRC0 is the beginning of the DMA0-locked SARAM half-buffer. */
    DMSA = 0x00 /* DMSRC0 */;
    dma0_locked = (DMSDN << 1) & (DSP_BUFFER_SIZE);
    dma0_unlocked = dma0_locked ^ (DSP_BUFFER_SIZE);
    
	dsp_level = 0;
    
    /* Start the SDRAM to SARAM copy */
	rebuffer();
}

/* This interupt handler runs every time a DMA transfer is complete from SDRAM 
 *  to the SARAM buffer.  It is used to update the SARAM buffer level 
 *  (dsp_level), the SDRAM buffer level (sdem_level) and to rebuffer if the dsp
 *  buffer is not full.
 */
interrupt void handle_dmac(void) {
    IFR = 1 << 11;
    
   	dsp_level+=last_size;
   	sdem_level+=last_size;
   	
    if(dsp_level<DSP_BUFFER_SIZE)
    {
		rebuffer();
	}
}

void dma_init(void) {
    /* Configure SARAM to McBSP DMA */
    
    /* Event XEVT0, 32-bit transfers, 0 frame count */
   DMSFC0 = 2 << 12 | 1 << 11; 
    
    /* Interrupts generated, Half and full buffer.
     * ABU mode, From data space with postincrement, to data space with no 
     *  change
     */
    DMMCR0 = 1 << 14 | 1 << 13 | 
             1 << 12 | 1 << 8 | 1 << 6 | 1; 
             
    /* Set the source (incrementing) location */
    DMSRC0 = (unsigned short)&data;
    
    /* Set the destination (static) location to the McBSP IIS interface */
    DMDST0 = (unsigned short)&DXR20; 
    
    /* Set the size of the buffer */
    DMCTR0 = sizeof(data);

    /* Setup DMA0 interrupts and start the transfer */
    DMPREC = 2 << 6;
}
