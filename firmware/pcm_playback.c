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
#if defined(HAVE_UDA1380)
#include "uda1380.h"
#elif defined(HAVE_WM8975)
#include "wm8975.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#endif
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

#ifdef HAVE_SPDIF_OUT
#define EBU_DEFPARM        ((7 << 12) | (3 << 8) | (1 << 5) | (5 << 2))
#endif
#define IIS_DEFPARM(freq)  ((freq << 12) | 0x300 | 4 << 2)
#define IIS_RESET          0x800

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
#ifdef HAVE_SPDIF_OUT
    EBU1CONFIG = IIS_RESET | EBU_DEFPARM;
#endif

    /* Set up DMA transfer  */
    SAR0 = ((unsigned long)addr); /* Source address */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer */

    /* Enable the FIFO and force one write to it */
    IIS2CONFIG = IIS_DEFPARM(pcm_freq);
    /* Also send the audio to S/PDIF */
#ifdef HAVE_SPDIF_OUT
    EBU1CONFIG = EBU_DEFPARM;
#endif
    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC | DMA_START;
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

    DCR0 = 0;
    DSR0 = 1;
    /* Reset the FIFO */
    IIS2CONFIG = IIS_RESET | IIS_DEFPARM(pcm_freq);
#ifdef HAVE_SPDIF_OUT
    EBU1CONFIG = IIS_RESET | EBU_DEFPARM;
#endif

    next_start = NULL;
    next_size = 0;
    pcm_paused = false;
}

/*
 * This function goes directly into the DMA buffer to calculate the left and
 * right peak values. To avoid missing peaks it tries to look forward two full
 * peek periods (2/HZ sec, 100% overlap), although it's always possible that
 * the entire period will not be visible. To reduce CPU load it only looks at
 * every third sample, and this can be reduced even further if needed (even
 * every tenth sample would still be pretty accurate).
 */

#define PEAK_SAMPLES  (44100*2/HZ)  /* 44100 samples * 2 / 100 Hz tick */
#define PEAK_STRIDE   3       /* every 3rd sample is plenty... */

void pcm_calculate_peaks(int *left, int *right)
{
    long samples = (BCR0 & 0xffffff) / 4;
    short *addr = (short *) (SAR0 & ~3);

    if (samples > PEAK_SAMPLES)
        samples = PEAK_SAMPLES;

    samples /= PEAK_STRIDE;

    if (left && right) {
        int left_peak = 0, right_peak = 0, value;    

        while (samples--) {
            if ((value = addr [0]) > left_peak)
                left_peak = value;
            else if (-value > left_peak)
                left_peak = -value;

            if ((value = addr [PEAK_STRIDE | 1]) > right_peak)
                right_peak = value;
            else if (-value > right_peak)
                right_peak = -value;

            addr += PEAK_STRIDE * 2;
        }

        *left = left_peak;
        *right = right_peak;
    }
    else if (left || right) {
        int peak_value = 0, value;

        if (right)
            addr += (PEAK_STRIDE | 1);

        while (samples--) {
            if ((value = addr [0]) > peak_value)
                peak_value = value;
            else if (-value > peak_value)
                peak_value = -value;

            addr += PEAK_STRIDE * 2;
        }

        if (left)
            *left = peak_value;
        else
            *right = peak_value;
    }
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

    get_more((unsigned char **)&start, (long *)&size);
    get_more(&next_start, &next_size);
    dma_start(start, size);
}

long pcm_get_bytes_waiting(void)
{
    return next_size + (BCR0 & 0xffffff);
}

void pcm_play_stop(void)
{
    if (pcm_playing) {
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
        IIS2CONFIG = IIS_DEFPARM(pcm_freq);
#ifdef HAVE_SPDIF_OUT
        EBU1CONFIG = EBU_DEFPARM;
#endif
        DCR0 |= DMA_EEXT | DMA_START;
    }
    else if(!pcm_paused && !play)
    {
        logf("pause");

        /* Disable DMA peripheral request. */
        DCR0 &= ~DMA_EEXT;
        IIS2CONFIG = IIS_RESET | IIS_DEFPARM(pcm_freq);
#ifdef HAVE_SPDIF_OUT
        EBU1CONFIG = IIS_RESET | EBU_DEFPARM;
#endif
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
       pcm_play_stop();
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
            pcm_play_stop();
            logf("DMA No Data:0x%04x", res);
        }
    }

    IPR |= (1<<14); /* Clear pending interrupt request */
}

void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;

    MPARK = 0x81;    /* PARK[1,0]=10 + BCR24BIT */
    DIVR0 = 54;      /* DMA0 is mapped into vector 54 in system.c */
    DMAROUTE = (DMAROUTE & 0xffffff00) | DMA0_REQ_AUDIO_1;
    DMACONFIG = 1;   /* DMA0Req = PDOR3 */

    /* Reset the audio FIFO */
    IIS2CONFIG = IIS_RESET;

    /* Enable interrupt at level 7, priority 0 */
    ICR6 = 0x1c;
    IMR &= ~(1<<14);      /* bit 14 is DMA0 */

    pcm_set_frequency(44100);

    /* Prevent pops (resets DAC to zero point) */
    IIS2CONFIG = IIS_DEFPARM(pcm_freq) | IIS_RESET;
    
#if defined(HAVE_UDA1380)
    /* Initialize default register values. */
    uda1380_init();
    
    /* Sleep a while so the power can stabilize (especially a long
       delay is needed for the line out connector). */
    sleep(HZ);

    /* Power on FSDAC and HP amp. */
    uda1380_enable_output(true);

    /* Unmute the master channel (DAC should be at zero point now). */
    uda1380_mute(false);
    
#elif defined(HAVE_TLV320)
    tlv320_init();
    tlv320_enable_output(true);
    sleep(HZ/4);
    tlv320_mute(false);
#endif

    
    /* Call dma_stop to initialize everything. */
    dma_stop();
}

#elif defined(HAVE_WM8975)

/* We need to unify this code with the uda1380 code as much as possible, but
   we will keep it separate during early development.
*/

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

    /* Disable playback for now */
    pcm_playing = false;
    return;

/* This is the uda1380 code */
#if 0
    /* Reset the audio FIFO */

    /* Set up DMA transfer  */
    SAR0 = ((unsigned long)addr); /* Source address */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer */

    /* Enable the FIFO and force one write to it */
    IIS2CONFIG = IIS_DEFPARM(pcm_freq);

    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_SINC | DMA_START;
#endif
}

/* Stops the DMA transfer and interrupt */
static void dma_stop(void)
{
    pcm_playing = false;

#if 0
/* This is the uda1380 code */
    DCR0 = 0;
    DSR0 = 1;
    /* Reset the FIFO */
    IIS2CONFIG = IIS_RESET | IIS_DEFPARM(pcm_freq);
#endif
    next_start = NULL;
    next_size = 0;
    pcm_paused = false;
}


void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;

    /* Initialize default register values. */
    wm8975_init();
    
    /* The uda1380 needs a sleep(HZ) here - do we need one? */

    /* Power on */
    wm8975_enable_output(true);

    /* Unmute the master channel (DAC should be at zero point now). */
    wm8975_mute(false);
    
    /* Call dma_stop to initialize everything. */
    dma_stop();
}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
    pcm_freq=frequency;
}

/* the registered callback function to ask for more mp3 data */
static void (*callback_for_more)(unsigned char**, long*) = NULL;

void pcm_play_data(void (*get_more)(unsigned char** start, long* size))
{
    unsigned char *start;
    long size;

    callback_for_more = get_more;

    get_more((unsigned char **)&start, (long *)&size);
    get_more(&next_start, &next_size);

    dma_start(start, size);
}

void pcm_play_stop(void)
{
    if (pcm_playing) {
        dma_stop();
    }
}

void pcm_play_pause(bool play)
{
    if(pcm_paused && play && next_size)
    {
        logf("unpause");
        /* We need to enable DMA here */
    }
    else if(!pcm_paused && !play)
    {
        logf("pause");
        /* We need to disable DMA here */
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

void pcm_calculate_peaks(int *left, int *right)
{
    *left=0;
    *right=0;
}

long pcm_get_bytes_waiting(void)
{
    return 0;
}

#elif CONFIG_CPU == PNX0101

/* TODO: Implement for iFP7xx
   For now, just implement some dummy functions.
*/

void pcm_init(void)
{

}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
}

void pcm_play_data(void (*get_more)(unsigned char** start, long* size))
{
    (void)get_more;
}

void pcm_play_stop(void)
{
}

void pcm_play_pause(bool play)
{
    (void)play;
}

bool pcm_is_paused(void)
{
    return false;
}

bool pcm_is_playing(void)
{
    return false;
}

void pcm_calculate_peaks(int *left, int *right)
{
    (void)left;
    (void)right;
}

long pcm_get_bytes_waiting(void)
{
    return 0;
}

#endif
