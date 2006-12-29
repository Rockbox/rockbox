/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "wm8975.h"
#include "file.h"

static int pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */

#define FIFO_COUNT ((IISFCON >> 6) & 0x01F)

/* number of bytes in FIFO */
#define IIS_FIFO_SIZE 64

/* Setup for the DMA controller */
#define DMA_CONTROL_SETUP  ((1<<31) | (1<<29) | (1<<23) | (1<<22) | (1<<20))

unsigned short * p;
size_t p_size;



/* DMA count has hit zero - no more data */
/* Get more data from the callback and top off the FIFO */
//void fiq(void) __attribute__ ((interrupt ("naked")));
void fiq(void) ICODE_ATTR __attribute__ ((interrupt ("FIQ")));
void fiq(void)
{
    /* clear any pending interrupt */
    SRCPND = (1<<19);

    /* Buffer empty.  Try to get more. */
    if (pcm_callback_for_more) 
    {
        pcm_callback_for_more((unsigned char**)&p, &p_size);
    }
    else 
    {
        /* callback func is missing? */
        pcm_play_dma_stop();
        return;
    }

    if (p_size)
    {
        /* set the new DMA values */
        DCON2 = DMA_CONTROL_SETUP | (p_size >> 1);
        DISRC2 = (int)p + 0x30000000;

        /* Re-Activate the channel */
        DMASKTRIG2 = 0x2; 
    }
    else
    {
        /* No more DMA to do */
        pcm_play_dma_stop();
    }
        
}



void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;
    pcm_callback_for_more = NULL;
    
    audiohw_init();
    audiohw_enable_output(true);
    audiohw_mute(true);

    /* cannot use the WM8975 defaults since our clock is not the same */
    /* the input master clock is 16.9344MHz - we can divide exact for that */
    audiohw_set_sample_rate( (0<<6) | (0x11 << 1) | (0<<0));

    /* init GPIO */
    GPCCON = (GPCCON & ~(3<<14)) | (1<<14);
    GPCDAT |= 1<<7;
    GPECON |= 0x2aa;

    /* Do no service DMA0 requests, yet */
    /* clear any pending int and mask it */
    INTMSK |= (1<<19);      /* mask the interrupt */
    SRCPND = (1<<19);       /* clear any pending interrupts */
    INTMOD |= (1<<19);      /* connect to FIQ */

}



void pcm_play_dma_start(const void *addr, size_t size)
{
    //FIXME
    //return;

    int i;
    
    /* sanity check: bad pointer or too small file */
    if ((NULL == addr) || (size & ~1) <= IIS_FIFO_SIZE) return;

    p = (unsigned short *)addr;
    p_size = size;


    /* Enable the IIS clock */
    CLKCON |= (1<<17);  

    /* IIS interface setup and set to idle */
    IISCON = (1<<5) | (1<<3);

    /* slave, transmit mode, 16 bit samples - 384fs - use 16.9344Mhz */ 
    IISMOD = (1<<9) | (1<<8) | (2<<6) | (1<<3) | (1<<2);

    /* connect DMA to the FIFO and enable the FIFO */
    IISFCON = (1<<15) | (1<<13);

    /* set DMA dest */
    DIDST2 = (int)&IISFIFO; 

    /* IIS is on the APB bus, INT when TC reaches 0, fixed dest addr */
    DIDSTC2 = 0x03;
    
    /* How many transfers to make - we transfer half-word at a time = 2 bytes */
    /* DMA control: CURR_TC int, single service mode, I2SSDO int, HW trig */
    /*     no auto-reload, half-word (16bit) */
    DCON2 = DMA_CONTROL_SETUP | (p_size / 2);

    /* set DMA source and options */
    DISRC2 = (int)p + 0x30000000;
    DISRCC2 = 0x00;  /* memory is on AHB bus, increment addresses */

    /* clear pending DMA interrupt */
    SRCPND = 1<<19;

    enable_fiq(fiq);

    /* unmask the DMA interrupt */
    INTMSK &= ~(1<<19);

    /* Activate the channel */
    DMASKTRIG2 = 0x2;
    
    /* turn off the idle */
    IISCON &= ~(1<<3);

    pcm_playing = true;

    /* start the IIS */
    IISCON |= (1<<0);

}



/* Disconnect the DMA and wait for the FIFO to clear */
void pcm_play_dma_stop(void)
{
    pcm_playing = false;

    /* mask the DMA interrupt */
    INTMSK |= (1<<19);

    /* De-Activate the channel */
    DMASKTRIG2 = 0x4;

    /* idle the IIS transmit */
    IISCON |= (1<<3);

    /* stop the IIS interface */
    IISCON &= ~(1<<0);

    /* Disconnect the IIS IIS clock */
    CLKCON &= ~(1<<17);  


    disable_fiq();

}



void pcm_play_pause_pause(void)
{
    /* idle */
    IISCON |= (1<<3);
}



void pcm_play_pause_unpause(void)
{
    /* no idle */
    IISCON &= ~(1<<3);
}



void pcm_set_frequency(unsigned int frequency)
{
    int sr_ctrl;

    switch(frequency)
    {
        case SAMPR_11:
            sr_ctrl = 0x19 << 1;
            break;
        case SAMPR_22:
            sr_ctrl = 0x1B << 1;
            break;
        default:
        case SAMPR_44:
            sr_ctrl = 0x11 << 1;
            break;
        case SAMPR_88:
            sr_ctrl = 0x1F << 1;
            break;
    }
    audiohw_set_sample_rate(sr_ctrl);
    pcm_freq = frequency;
}



size_t pcm_get_bytes_waiting(void)
{
    return (DSTAT2 & 0xFFFFF) * 2;
}



/* dummy functions for those not actually supporting all this yet */
void pcm_apply_settings(bool reset)
{
    (void)reset;
}

void pcm_set_monitor(int monitor)
{
    (void)monitor;
}
/** **/

void pcm_mute(bool mute)
{
    audiohw_mute(mute);
    if (mute)
        sleep(HZ/16);
}

/*
 * This function goes directly into the DMA buffer to calculate the left and
 * right peak values. To avoid missing peaks it tries to look forward two full
 * peek periods (2/HZ sec, 100% overlap), although it's always possible that
 * the entire period will not be visible. To reduce CPU load it only looks at
 * every third sample, and this can be reduced even further if needed (even
 * every tenth sample would still be pretty accurate).
 */

/* Check for a peak every PEAK_STRIDE samples */
#define PEAK_STRIDE   3
/* Up to 1/50th of a second of audio for peak calculation */
/* This should use NATIVE_FREQUENCY, or eventually an adjustable freq. value */
#define PEAK_SAMPLES  (44100/50)
void pcm_calculate_peaks(int *left, int *right)
{
    short *addr;
    short *end;
    {
        size_t samples = p_size / 4;
        addr = p;

        if (samples > PEAK_SAMPLES)
            samples = PEAK_SAMPLES - (PEAK_STRIDE - 1);
        else
            samples -= MIN(PEAK_STRIDE - 1, samples);

        end = &addr[samples * 2];
    }

    if (left && right) {
        int left_peak = 0, right_peak = 0;

        while (addr < end) {
            int value;
            if ((value = addr [0]) > left_peak)
                left_peak = value;
            else if (-value > left_peak)
                left_peak = -value;

            if ((value = addr [PEAK_STRIDE | 1]) > right_peak)
                right_peak = value;
            else if (-value > right_peak)
                right_peak = -value;

            addr = &addr[PEAK_STRIDE * 2];
        }

        *left = left_peak;
        *right = right_peak;
    }
    else if (left || right) {
        int peak_value = 0, value;

        if (right)
            addr += (PEAK_STRIDE | 1);

        while (addr < end) {
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
