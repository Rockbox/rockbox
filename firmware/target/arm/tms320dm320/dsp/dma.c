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
#include "ipc.h"

/* Size of data buffer in words (16 bit) */
#define DSP_BUFFER_SIZE (0x1000) 

/* Put the "data" buffer in it's own .dma section so that it can
 *  be handled in the linker.cmd. */
#pragma DATA_SECTION (data, ".dma")

/* This is the "data" buffer on the DSP side used for SARAM to McBSP (IIS) */
static signed short     data[DSP_BUFFER_SIZE];

/* These two describe the location of the buffer on the ARM (set in DSPHINT) */
volatile unsigned short sdem_addrh;
volatile unsigned short sdem_addrl;

/* This is the size of the ARM buffer (set in DSPHINT) */
volatile unsigned short sdem_dsp_size;

/* These two variables keep track of the buffer level in the DSP, dsp_level,
 *  (SARAM to McBSP) and the level on the ARM buffer (sdem_level). 
 * sdem_level is used in the main firmware to keep track of the current 
 *  playback status.  dsp_level is only used in this function. */
static unsigned short   dsp_level;
volatile unsigned short sdem_level;

/* This is used to keep track of the last SDRAM to SARAM transfer */
static unsigned short   last_size;

/* This tells us which half of the DSP buffer (data) is free */
static unsigned short   dma0_unlocked;

/* This is used by the ARM to flag playback status and start/stop the DMA 
 *  transfers. */
volatile unsigned short dma0_stopped;

/* This is used to effectively flag whether the ARM has new data ready or not */
short waiting;


/* rebuffer sets up the next SDRAM to SARAM transfer and tells the ARM when DMA
 *  needs a new buffer.
 *
 * Note: The upper limit on larger buffers is the size of a short.  If larger 
 *  buffer sizes are needed the code on the ARM side needs to be changed to 
 *  update a full long.
 */
void rebuffer(void)
{
    unsigned long sdem_addr;

    if(dma0_stopped==1 || dma0_stopped==2) /* Stop / Pause */
    {
        /* Stop MCBSP DMA0 */
        DMPREC      &=  0xFFFE;
        /* Shut the transmitter down */
        audiohw_stop();
        
        /* Stop the HPIB transfer if it is running */
        DMA_TRG     =   0; 
        
        /* Reset the following variables for DMA restart */
        sdem_level  =   0;
        dsp_level   =   0;
        last_size   =   0; 
        
        return;
    }
    
    /* If the sdem_level is equal to the buffer size the ARM code gave
     *  (sdem_dsp_size) then reset the size and ask the arm for another buffer
     */
    if(sdem_level == sdem_dsp_size)
    {
        sdem_level=0;

        /* Get a new buffer (location and size) from ARM */
        status.msg = MSG_REFILL;
        waiting=1;

        /* trigger DSPHINT on the ARM */
        int_arm();
    }

    if(!waiting)
    {
        /* Size is in bytes (but forced 32 bit transfers).  Comparison is
         *  against DSP_BUFFER_SIZE because it is in words and this needs to
         *  compare against half the total size in bytes. */
        if( dsp_level + sdem_dsp_size - sdem_level > DSP_BUFFER_SIZE)
        {
            last_size = DSP_BUFFER_SIZE - dsp_level;
        }
        else
        {
            last_size = sdem_dsp_size - sdem_level;
        }

        /* DSP addresses are 16 bit (word). dsp_level is in bytes so it needs to 
         *  be converted to words. */
        DSP_ADDRL = (unsigned short)data + dma0_unlocked + (dsp_level >> 1);
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
 * interupt occurs except for the initial start. */
interrupt void handle_dma0(void) 
{
    /* Byte offset to half-buffer locked by DMA0.
            0 for top, DSP_BUFFER_SIZE/2 for bottom */
    unsigned short dma0_locked;

    IFR = 1 << 6;

    /* DMSRC0 is the beginning of the DMA0-locked SARAM half-buffer. */
    DMSA = 0x00 /* DMSRC0 (banked register, see page 133 of SPRU302B */;
    
    /* Note that these address offsets (dma0_locked and dma0_unlocked are in 
     *  words. */
    dma0_locked     = DMSDN         & (DSP_BUFFER_SIZE>>1);
    dma0_unlocked   = dma0_locked   ^ (DSP_BUFFER_SIZE>>1);

    dsp_level = 0;
    
    /* Start the SDRAM to SARAM copy */
    rebuffer();
}

/* This interupt handler runs every time a DMA transfer is complete from SDRAM 
 *  to the SARAM buffer.  It is used to update the SARAM buffer level 
 *  (dsp_level), the SDRAM buffer level (sdem_level) and to rebuffer if the dsp
 *  buffer is not full. */
interrupt void handle_dmac(void) {
    IFR         = 1 << 11; /* Clear interrupt */

    /* dsp_level and sdem_level are in bytes */
    dsp_level   += last_size;
    sdem_level  += last_size;

    /* compare to DSP_BUFFER_SIZE without a divide because it is in words and 
     *  we want half the total size in bytes. */
    if(dsp_level < DSP_BUFFER_SIZE)
    {
        rebuffer();
    }
}

void dma_init(void) {
    /* Initialize some of the global variables to known values avoiding the 
     *  .cinit section. */
    dsp_level       =   0;
    sdem_level      =   0;

    last_size       =   0;
    dma0_unlocked   =   0;
    dma0_stopped    =   1;
    
    waiting         =   0;

    /* Configure SARAM to McBSP DMA */

    /* Event XEVT0, 32-bit transfers, 0 frame count */
    DMSFC0 = 2 << 12 | 1 << 11; 

    /* Interrupts generated, Half and full buffer.
     * ABU mode, From data space with postincrement, to data space with no 
     *  change
     */
    DMMCR0 =    1 << 14 | 1 << 13 | 
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
