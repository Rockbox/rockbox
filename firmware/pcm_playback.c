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
static bool pcm_paused;
static int pcm_freq = 0x6; /* 44.1 is default */

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
static void dma_start(const void *addr, long size)
{
    pcm_playing = true;

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* Reset the audio FIFO */
    IIS2CONFIG = 0x800;
    
    /* Set up DMA transfer  */
    SAR0 = ((unsigned long)addr); /* Source address */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer */

    /* Enable the FIFO and force one write to it */
    IIS2CONFIG = (pcm_freq << 12) | 0x300;
    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC | DMA_START;
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

    /* Reset the FIFO */
    IIS2CONFIG = 0x800;
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
        break;
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

void pcm_play_pause(bool play)
{
    if(pcm_paused && play)
    {
        /* Enable the FIFO and force one write to it */
        IIS2CONFIG = (pcm_freq << 12) | 0x300;
        DCR0 |= DMA_START;
        
        pcm_paused = false;
    }
    else if(!pcm_paused && !play)
    {
        IIS2CONFIG = 0x800;
        pcm_paused = true;
    }
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

    /* Stop on error */
    if(res & 0x70)
    {
       dma_stop();
    }
    else
    {
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
    }
    
    IPR |= (1<<14); /* Clear pending interrupt request */
}

void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;
    
    uda1380_init();

    BUSMASTER_CTRL = 0x81; /* PARK[1,0]=10 + BCR24BIT */
    DIVR0 = 54;            /* DMA0 is mapped into vector 54 in system.c */
    DMAROUTE = (DMAROUTE & 0xffffff00) | DMA0_REQ_AUDIO_1;
    DMACONFIG = 1;   /* DMA0Req = PDOR3 */

    /* Reset the audio FIFO */
    IIS2CONFIG = 0x800;
    
    /* Enable interrupt at level 7, priority 0 */
    ICR4 = (ICR4 & 0xffff00ff) | 0x00001c00;
    IMR &= ~(1<<14);      /* bit 14 is DMA0 */

    pcm_set_frequency(44100);
}


#define NUM_PCM_BUFFERS 16 /* Must be a power of 2 */
#define NUM_PCM_BUFFERS_MASK (NUM_PCM_BUFFERS - 1)

struct pcmbufdesc
{
    void *addr;
    int size;
    void (*callback)(void); /* Call this when the buffer has been played */
} pcmbuffers[NUM_PCM_BUFFERS];

int pcmbuf_read_index;
int pcmbuf_write_index;
int pcmbuf_unplayed_bytes;
int pcmbuf_watermark;
void (*pcmbuf_watermark_callback)(int bytes_left);

int pcm_play_num_used_buffers(void)
{
    return (pcmbuf_write_index - pcmbuf_read_index) & NUM_PCM_BUFFERS_MASK;
}

void pcm_play_set_watermark(int numbytes, void (*callback)(int bytes_left))
{
    pcmbuf_watermark = numbytes;
    pcmbuf_watermark_callback = callback;
}

bool pcm_play_add_chunk(void *addr, int size, void (*callback)(void))
{
    /* We don't use the last buffer, since we can't see the difference
       between the full and empty condition */
    if(pcm_play_num_used_buffers() < (NUM_PCM_BUFFERS - 1))
    {
        pcmbuffers[pcmbuf_write_index].addr = addr;
        pcmbuffers[pcmbuf_write_index].size = size;
        pcmbuffers[pcmbuf_write_index].callback = callback;
        pcmbuf_write_index = (pcmbuf_write_index+1) & NUM_PCM_BUFFERS_MASK;
        pcmbuf_unplayed_bytes += size;
        return true;
    }
    else
        return false;
}

void pcm_play_init(void)
{
    pcmbuf_read_index = 0;
    pcmbuf_write_index = 0;
    pcmbuf_unplayed_bytes = 0;
    pcm_play_set_watermark(0x10000, NULL);
}

static int last_chunksize = 0;

static void pcm_play_callback(unsigned char** start, long* size)
{
    struct pcmbufdesc *desc = &pcmbuffers[pcmbuf_read_index];
    int sz;

    pcmbuf_unplayed_bytes -= last_chunksize;
    
    if(desc->size == 0)
    {
        /* The buffer is finished, call the callback function */
        if(desc->callback)
            desc->callback();

        /* Advance to the next buffer */
        pcmbuf_read_index = (pcmbuf_read_index + 1) & NUM_PCM_BUFFERS_MASK;
        desc = &pcmbuffers[pcmbuf_read_index];
    }
    
    if(pcm_play_num_used_buffers())
    {
        /* Play max 64K at a time */
        sz = MIN(desc->size, 32768);
        *start = desc->addr;
        *size = sz;

        /* Update the buffer descriptor */
        desc->size -= sz;
        desc->addr += sz;

        last_chunksize = sz;
    }
    else
    {
        /* No more buffers */
        *size = 0;
    }
#if 0
    if(pcmbuf_unplayed_bytes <= pcmbuf_watermark)
    {
        if(pcmbuf_watermark_callback)
        {
            pcmbuf_watermark_callback(pcmbuf_unplayed_bytes);
        }
    }
#endif
}

void pcm_play_start(void)
{
    struct pcmbufdesc *desc = &pcmbuffers[pcmbuf_read_index];
    int size;
    char *start;
    
    if(!pcm_is_playing())
    {
        size = MIN(desc->size, 32768);
        start = desc->addr;
        pcm_play_data(start, size, pcm_play_callback);
        last_chunksize = size;
        desc->size -= size;
        desc->addr += size;
    }
}
