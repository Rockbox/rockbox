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
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "file.h"
#include "mmu-meg-fx.h"

#define GIGABEAT_11025HZ     (0x19 << 1)
#define GIGABEAT_22050HZ     (0x1b << 1)
#define GIGABEAT_44100HZ     (0x11 << 1)
#define GIGABEAT_88200HZ     (0x1f << 1)

static int pcm_freq = 0; /* 44.1 is default */
static int sr_ctrl = 0;
#define FIFO_COUNT ((IISFCON >> 6) & 0x01F)

/* number of bytes in FIFO */
#define IIS_FIFO_SIZE 64

/* Setup for the DMA controller */
#define DMA_CONTROL_SETUP  ((1<<31) | (1<<29) | (1<<23) | (1<<22) | (1<<20))

/* DMA count has hit zero - no more data */
/* Get more data from the callback and top off the FIFO */
/* Uses explicitly coded prologue/epilogue code to get around complier bugs
   in order to be able to use the stack */
void fiq_handler(void) __attribute__((naked));

static void _pcm_apply_settings(void)
{
    static int last_freqency = 0;

    if (pcm_freq != last_freqency)
    {
        last_freqency = pcm_freq;
        audiohw_set_frequency(sr_ctrl);
    }
}

void pcm_apply_settings(void)
{
    int oldstatus = set_fiq_status(FIQ_DISABLED);
    _pcm_apply_settings();
    set_fiq_status(oldstatus);
}

void pcm_init(void)
{
    pcm_playing = false;
    pcm_paused = false;
    pcm_callback_for_more = NULL;

    pcm_set_frequency(SAMPR_44);

    audiohw_init();

    /* init GPIO */
    GPCCON = (GPCCON & ~(3<<14)) | (1<<14);
    GPCDAT |= 1<<7;
    GPECON |= 0x2aa;

    /* Do not service DMA requests, yet */
    /* clear any pending int and mask it */
    INTMSK |= (1<<19);      /* mask the interrupt */
    SRCPND = (1<<19);       /* clear any pending interrupts */
    INTMOD |= (1<<19);      /* connect to FIQ */

}

void pcm_postinit(void)
{
    audiohw_postinit();
    pcm_apply_settings();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* sanity check: bad pointer or too small file */
    if (NULL == addr || size <= IIS_FIFO_SIZE) return;

    disable_fiq();

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
    DCON2 = DMA_CONTROL_SETUP | (size / 2);

    /* set DMA source and options */
    DISRC2 = (unsigned long)addr + 0x30000000;
    DISRCC2 = 0x00;  /* memory is on AHB bus, increment addresses */

    /* clear pending DMA interrupt */
    SRCPND = 1<<19;

    pcm_playing = true;

    _pcm_apply_settings();

    /* unmask the DMA interrupt */
    INTMSK &= ~(1<<19);

    /* Flush any pending writes */
    clean_dcache_range(addr, size);

    /* Activate the channel */
    DMASKTRIG2 = 0x2;

    /* turn off the idle */
    IISCON &= ~(1<<3);

    /* start the IIS */
    IISCON |= (1<<0);

    enable_fiq();
}

static void pcm_play_dma_stop_fiq(void)
{
    /* mask the DMA interrupt */
    INTMSK |= (1<<19);

    /* are we playing? wait for the chunk to finish */
    if (pcm_playing)
    {
        /* wait for the FIFO to empty before turning things off */
        while (IISCON & (1<<7))  ;

        pcm_playing = false;
        pcm_paused = false;
    }

    /* De-Activate the DMA channel */
    DMASKTRIG2 = 0x4;

    /* Disconnect the IIS clock */
    CLKCON &= ~(1<<17);
}

void fiq_handler(void)
{
    /* r0-r7 are probably not all used by GCC but there's no way to know
       otherwise this whole thing must be assembly */
    asm volatile ("stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                  "sub   sp, sp, #8           \n"); /* Reserve stack */
    register pcm_more_callback_type get_more;   /* No stack for this */
    unsigned char                  *next_start; /* sp + #0 */
    size_t                          next_size;  /* sp + #4 */

    /* clear any pending interrupt */
    SRCPND = (1<<19);

    /* Buffer empty.  Try to get more. */
    get_more = pcm_callback_for_more;
    if (get_more == NULL)
    {
        /* Callback missing */
        pcm_play_dma_stop_fiq();
        goto fiq_exit;
    }

    next_size = 0;
    get_more(&next_start, &next_size);

    if (next_size == 0)
    {
        /* No more DMA to do */
        pcm_play_dma_stop_fiq();
        goto fiq_exit;
    }

    /* Flush any pending cache writes */
    clean_dcache_range(next_start, next_size);

    /* set the new DMA values */
    DCON2 = DMA_CONTROL_SETUP | (next_size >> 1);
    DISRC2 = (unsigned long)next_start + 0x30000000;

    /* Re-Activate the channel */
    DMASKTRIG2 = 0x2;

fiq_exit:
    asm volatile("add   sp, sp, #8           \n"   /* Cleanup stack   */
                 "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                 "subs  pc, lr, #4           \n"); /* Return from FIQ */
}

/* Disconnect the DMA and wait for the FIFO to clear */
void pcm_play_dma_stop(void)
{
    disable_fiq();
    pcm_play_dma_stop_fiq();
}


void pcm_play_pause_pause(void)
{
    /* stop servicing refills */
    int oldstatus = set_fiq_status(FIQ_DISABLED);
    INTMSK |= (1<<19);
    set_fiq_status(oldstatus);
}



void pcm_play_pause_unpause(void)
{
    /* refill buffer and keep going */
    int oldstatus = set_fiq_status(FIQ_DISABLED);
    _pcm_apply_settings();
    INTMSK &= ~(1<<19);
    set_fiq_status(oldstatus);
}

void pcm_set_frequency(unsigned int frequency)
{
    switch(frequency)
    {
        case SAMPR_11:
            sr_ctrl = GIGABEAT_11025HZ;
            break;
        case SAMPR_22:
            sr_ctrl = GIGABEAT_22050HZ;
            break;
        default:
            frequency = SAMPR_44;
        case SAMPR_44:
            sr_ctrl = GIGABEAT_44100HZ;
            break;
        case SAMPR_88:
            sr_ctrl = GIGABEAT_88200HZ;
            break;
    }

    pcm_freq = frequency;
}



size_t pcm_get_bytes_waiting(void)
{
    return (DSTAT2 & 0xFFFFF) * 2;
}

#if 0
void pcm_set_monitor(int monitor)
{
    (void)monitor;
}
#endif
/** **/

void pcm_mute(bool mute)
{
    audiohw_mute(mute);
    if (mute)
        sleep(HZ/16);
}

/**
 * Return playback peaks - Peaks ahead in the DMA buffer based upon the
 *                         calling period to attempt to compensate for
 *                         delay.
 */
void pcm_calculate_peaks(int *left, int *right)
{
    static unsigned long last_peak_tick = 0;
    static unsigned long frame_period   = 0;
    static int peaks_l = 0, peaks_r = 0;

    /* Throttled peak ahead based on calling period */
    unsigned long period = current_tick - last_peak_tick;

    /* Keep reasonable limits on period */
    if (period < 1)
        period = 1;
    else if (period > HZ/5)
        period = HZ/5;

    frame_period = (3*frame_period + period) >> 2;

    last_peak_tick = current_tick;

    if (pcm_playing && !pcm_paused)
    {
        unsigned long *addr = (unsigned long *)DCSRC2;
        long samples = DSTAT2;
        long samp_frames;

        samples    &= 0xFFFFE;
        samp_frames = frame_period*pcm_freq/(HZ/2);
        samples     = MIN(samp_frames, samples) >> 1;

        if (samples > 0)
        {
            long peak_l   = 0, peak_r   = 0;
            long peaksq_l = 0, peaksq_r = 0;

            addr -= 0x30000000 >> 2;
            addr = (long *)((long)addr & ~3);

            do
            {
                long value = *addr;
                long ch, chsq;

                ch   = (int16_t)value;
                chsq = ch*ch;
                if (chsq > peaksq_l)
                    peak_l = ch, peaksq_l = chsq;

                ch   = value >> 16;
                chsq = ch*ch;
                if (chsq > peaksq_r)
                    peak_r = ch, peaksq_r = chsq;

                addr += 4;
            }
            while ((samples -= 4) > 0);

            peaks_l = abs(peak_l);
            peaks_r = abs(peak_r);
        }
    }
    else
    {
        peaks_l = peaks_r = 0;
    }

    if (left)
        *left = peaks_l;

    if (right)
        *right = peaks_r;
} /* pcm_calculate_peaks */
