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
#ifndef SIMULATOR
#include "cpu.h"
#include "i2c.h"
#include "uda1380.h"
#include "system.h"
#endif
#include "logf.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "pcm_playback.h"
#include "lcd.h"
#include "button.h"
#include "file.h"
#include "buffer.h"
#include "sprintf.h"
#include "button.h"
#include <string.h>

#ifdef HAVE_UDA1380

static bool pcm_playing;
static bool pcm_paused;
static int pcm_freq = 0x6; /* 44.1 is default */

static unsigned char *next_start;
static long next_size;

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
static void dma_start(const void *addr, long size)
{
    pcm_playing = true;

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* Reset the audio FIFO */
    IIS2CONFIG = 0x800;
    EBU1CONFIG = 0x800;

    /* Set up DMA transfer  */
    SAR0 = ((unsigned long)addr); /* Source address */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer */

    /* Enable the FIFO and force one write to it */
    IIS2CONFIG = (pcm_freq << 12) | 0x300 | 4 << 2;
    /* Also send the audio to S/PDIF */
    EBU1CONFIG = 7 << 12 |  3 << 8 | 5 << 2;
    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC | DMA_START;
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

    DCR0 = 0;
    /* Reset the FIFO */
    IIS2CONFIG = 0x800;
    EBU1CONFIG = 0x800;

    next_start = NULL;
    next_size = 0;
    pcm_paused = false;
}

#define PEEK_SAMPLE_COUNT 64
static long calculate_channel_peak_average(int channel, unsigned short *addr,
                                           long size)
{
    int i;
    long min, max;
    int count, min_count, max_count;
    unsigned long average, zero_point;

    addr = &addr[channel];
    average = 0;
    
    if (pcm_playing && !pcm_paused && addr != NULL && size)
    {
        /* Calculate the zero point and remove DC offset (should be around 32768) */
        zero_point = 0;
        for (i = 0; i < size; i++)
            zero_point += addr[i*2];
        zero_point /= i;

        /*for (i = 0; i < size; i++) {
            long peak = addr[i*2] - 32768;
            if (peak < 0)
                peak = 0;
            average += peak;
        }
        average /= i;*/
        
        count = 0;

        while (size > PEEK_SAMPLE_COUNT)
        {
            min = zero_point;
            max = zero_point;
            min_count = 1;
            max_count = 1;
            
            for (i = 0; i < PEEK_SAMPLE_COUNT; i++)
            {
                unsigned long value = *addr;
                if (value < zero_point) {
                    min += value;
                    min_count++;
                }
                if (value > zero_point) {
                    max += value;
                    max_count++;
                }
                addr = &addr[2];
            }

            min /= min_count;
            max /= max_count;
            
            size -= PEEK_SAMPLE_COUNT;
            average += (max - min) / 2;
            //average += (max - zero_point) + (zero_point - min);
            //average += zero_point - min;
            count += 1;
        }

        if (count)
        {
            average /= count;
            /* I don't know why this is needed. Should be fixed. */
            average = zero_point - average;
        }
    }

    return average;
}

void pcm_calculate_peaks(int *left, int *right)
{
    unsigned short *addr = (unsigned short *)SAR0;
    long size = MIN(512, BCR0 / 2);
        
    if (left != NULL)
        *left = calculate_channel_peak_average(0, addr, size);
    if (right != NULL)
        *right = calculate_channel_peak_average(1, addr, size);;
}

/* sets frequency of input to DAC */
void pcm_set_frequency(unsigned int frequency)
{
    switch(frequency)
    {
    case 11025:
        pcm_freq = 0x4;
        uda1380_set_nsorder(3);
        break;
    case 22050:
        pcm_freq = 0x6;
        uda1380_set_nsorder(3);
        break;
    case 44100:
    default:
        pcm_freq = 0xC;
        uda1380_set_nsorder(5);
        break;
    }
}

/* the registered callback function to ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, long*) = NULL;

void pcm_play_data(void (*get_more)(unsigned char** start, long* size))
{
    unsigned char *start;
    long size;
    
    callback_for_more = get_more;
    
    /** FIXME: This is a temporary fix to prevent playback glitches when
      * playing the first file. We will just drop the first frame to prevent
      * that problem from occurring.
      * Some debug data:
      *   - This problem will occur only when the first file.
      *   - First frame will be totally corrupt and the song will begin
      *     from the next frame. But at the next time (when the bug has
      *     already happened), the song will start from first frame.
      *   - Dropping some frames directly from (mpa) codec will also
      *     prevent the problem from happening. So it's unlikely you can
      *     find the explanation for this bug from this file.
      */
    get_more((unsigned char **)&start, (long *)&size); // REMOVE THIS TO TEST
    get_more((unsigned char **)&start, (long *)&size);
    get_more(&next_start, &next_size);
    dma_start(start, size);
    uda1380_mute(false);
}

void pcm_play_stop(void)
{
    if (pcm_playing) {
        uda1380_mute(true);
        dma_stop();
    }
}

void pcm_play_pause(bool play)
{
    if(pcm_paused && play && next_size)
    {
        logf("unpause");
        /* Reset chunk size so dma has enough data to fill the fifo. */
        /* This shouldn't be needed anymore. */
        //SAR0 = (unsigned long)next_start;
        //BCR0 = next_size;
        /* Enable the FIFO and force one write to it */
        IIS2CONFIG = (pcm_freq << 12) | 0x300 | 4 << 2;
        EBU1CONFIG = 7 << 12 |  3 << 8 | 5 << 2;
        DCR0 |= DMA_EEXT | DMA_START;
        
        uda1380_mute(false);
    }
    else if(!pcm_paused && !play)
    {
        logf("pause");
        uda1380_mute(true);
        
        /* Disable DMA peripheral request. */
        DCR0 &= ~DMA_EEXT;
        IIS2CONFIG = 0x800;
        EBU1CONFIG = 0x800;
    }
    pcm_paused = !play;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

/* DMA0 Interrupt is called when the DMA has finished transfering a chunk */
void DMA0(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA0(void)
{
    int res = DSR0;

    DSR0 = 1;    /* Clear interrupt */
    DCR0 &= ~DMA_EEXT;
    
    /* Stop on error */
    if(res & 0x70)
    {
       dma_stop();
       logf("DMA Error:0x%04x", res);
    }
    else
    {
        if(next_size)
        {
            SAR0 = (unsigned long)next_start;  /* Source address */
            BCR0 = next_size;                  /* Bytes to transfer */
            DCR0 |= DMA_EEXT;
            if (callback_for_more)
                callback_for_more(&next_start, &next_size);
        }
        else
        {
            /* Finished playing */
            dma_stop();
            logf("DMA No Data:0x%04x", res);
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
    
    /* Turn on headphone power with audio output muted. */
    uda1380_mute(true);
    sleep(HZ/4);
    uda1380_enable_output(true);
    
    /* Call dma_stop to initialize everything. */
    dma_stop();
}

#endif /* HAVE_UDA1380 */
