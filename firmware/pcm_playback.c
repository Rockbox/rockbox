/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "panic.h"
#include <kernel.h>
#include "pcm_playback.h"
#ifndef SIMULATOR
#include "cpu.h"
#include "i2c.h"
#include "uda1380.h"
#include "system.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lcd.h"
#include "button.h"
#include "file.h"
#include "buffer.h"

#include "sprintf.h"
#include "button.h"
#include <string.h>

static bool pcm_playing;
static int pcm_freq = 0x6; // 44.1 in default

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
static void dma_start(const void *addr_r, long size)
{   
    pcm_playing = 1;
    int i;
    
    int align;
    align = 4;

    void* addr = (void*)((((unsigned int)addr_r) >> 2) << 2); // always align data, never pass unaligned data
    size = (size >> 2) << 2; // size shoudl also be always multiple of 4

    BUSMASTER_CTRL = 0x81; /* PARK[1,0]=10 + BCR24BIT */

    /* Set up DMA transfer  */
    DIVR0 = 54;          /* DMA0 is mapped into vector 54 in system.c */
    SAR0 = ((unsigned long)addr) + align*4;         /* Source address */
    DAR0 = (unsigned long)&PDOR3;       /* Destination address */
    BCR0 = size-(align*4);                        /* Bytes to transfer */
    DMAROUTE = (DMAROUTE & 0xffffff00) | DMA0_REQ_AUDIO_1;
    DMACONFIG = 1;   /* Enable DMA0Req => set DMAROUTE |= DMA0_REQ_AUDIO_1 */

    /* Start transfer when requested */
    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC;

    /* Enable interrupt at level 7, priority 0 */
    ICR4 = (ICR4 & 0xffff00ff) | 0x00001c00;
    IMR &= ~(1<<14);      /* bit 14 is DMA0 */

    IIS2CONFIG = (pcm_freq << 12) | 0x300;   /* CLOCKSEL for right frequency +  data source = PDOR3 */

    for(i = 0; i < align; i++)
        PDOR3 = ((unsigned int*)(addr))[i]; /* These are needed to generate FIFO empty request to DMA.. */
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = 0;
    DCR0 = 0;
                       
/*    DMAROUTE &= 0xffffff00;
    DMACONFIG = 0;*/

    IIS2CONFIG = 0x800;
    
    /* Disable DMA0 interrupt */
    IMR |= (1<<14);
    ICR4 &= 0xffff00ff;
}


/* set volume of the main channel */
void pcm_set_volume(int volume)
{
   if(volume > 0)
   {
       uda1380_mute(0);
       uda1380_setvol(0xff - volume); 
   }
   else
   {
       uda1380_mute(1);
   } 
}

/* sets frequency of input to DAC */
void pcm_set_frequency(unsigned int frequency)
{
   switch(frequency)
   {
    case 11025:
        pcm_freq = 0x2;
        break;
    case 22050:
        pcm_freq = 0x4;
        break;
    case 44100:
        pcm_freq = 0x6;
        break;
    default:
        pcm_freq = 0x6;
   }
}

/* the registered callback function to ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, long*) = NULL;

void pcm_play_data(const unsigned char* start, int size,
                   void (*get_more)(unsigned char** start, long* size))
{
    callback_for_more = get_more;

    dma_start(start, size);
}

void pcm_play_stop(void)
{
    dma_stop();
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

/* DMA0 Interrupt is called when the DMA has finished transfering a chunk */
void DMA0(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA0(void)
{
    unsigned char* start;
    long size = 0;

    int res = DSR0;

    DSR0 = 1;    /* Clear interrupt */

    if(res == 0x41)
    {
       dma_stop();
    }

    if (callback_for_more)
    {
        callback_for_more(&start, &size);
    }
    
    if(size)
    {
        SAR0 = (unsigned long)start;  /* Source address */
        BCR0 = size;                  /* Bytes to transfer */
    }
    else
    {
        /* Finished playing */
        dma_stop();
    }
    
    IPR |= (1<<14); /* Clear pending interrupt request */
}
