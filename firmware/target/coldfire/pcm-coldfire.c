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
#if defined(HAVE_UDA1380)
#include "uda1380.h"
#elif defined(HAVE_TLV320)
#include "tlv320.h"
#endif
#if defined(HAVE_SPDIF_IN) || defined(HAVE_SPDIF_OUT)
#include "spdif.h"
#endif

/* peaks */
static unsigned long *rec_peak_addr;
enum
{
    PLAY_PEAK_LEFT = 0,
    PLAY_PEAK_RIGHT,
    REC_PEAK_LEFT,
    REC_PEAK_RIGHT
};
static int peaks[4]; /* p-l, p-r, r-l, r-r */

#define IIS_DEFPARM(output)     ( (freq_ent[FPARM_CLOCKSEL] << 12) | \
                                  (output) | \
                                  (4 << 2) )  /* 64 bit clocks / word clock */
#define IIS_RESET       0x800

#ifdef IAUDIO_X5
#define SET_IIS_CONFIG(x) IIS1CONFIG = (x);
#define IIS_CONFIG        IIS1CONFIG
#define PLLCR_SET_AUDIO_BITS_DEFPARM \
            ((freq_ent[FPARM_CLSEL] << 28) | (1 << 22))
#else
#define SET_IIS_CONFIG(x) IIS2CONFIG = (x);
#define IIS_CONFIG        IIS2CONFIG
#define PLLCR_SET_AUDIO_BITS_DEFPARM \
            ((freq_ent[FPARM_CLSEL] << 28) | (3 << 22))
#endif

/** Sample rates **/
#define FPARM_CLOCKSEL       0
#define FPARM_CLSEL          1
#define FPARM_FSEL           2
#if CONFIG_CPU == MCF5249 && defined(HAVE_UDA1380)
static const unsigned char pcm_freq_parms[HW_NUM_FREQ][3] =
{
    [HW_FREQ_88] = { 0x0c, 0x01, 0x03 },
    [HW_FREQ_44] = { 0x06, 0x01, 0x02 },
    [HW_FREQ_22] = { 0x04, 0x02, 0x01 },
    [HW_FREQ_11] = { 0x02, 0x02, 0x00 },
};
#endif

#if CONFIG_CPU == MCF5250 && defined(HAVE_TLV320)
static const unsigned char pcm_freq_parms[HW_NUM_FREQ][3] =
{
    [HW_FREQ_88] = { 0x0c, 0x01, 0x02 },
    [HW_FREQ_44] = { 0x06, 0x01, 0x01 },
    [HW_FREQ_22] = { 0x04, 0x01, 0x00 },
    [HW_FREQ_11] = { 0x02, 0x02, 0x00 },
};
#endif

static int                  pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */
static const unsigned char *freq_ent = pcm_freq_parms[HW_FREQ_DEFAULT];

/* set frequency used by the audio hardware */
void pcm_set_frequency(unsigned int frequency)
{
    int index;

    switch(frequency)
    {
        case SAMPR_11:
            index = HW_FREQ_11;
            break;
        case SAMPR_22:
            index = HW_FREQ_22;
            break;
        default:
        case SAMPR_44:
            index = HW_FREQ_44;
            break;
        case SAMPR_88:
            index = HW_FREQ_88;
            break;
    }

    /* remember table entry and rate */
    freq_ent = pcm_freq_parms[index];
    pcm_freq = hw_freq_sampr[index];
} /* pcm_set_frequency */

/* apply audio settings */
void pcm_apply_settings(bool reset)
{
    static int last_pcm_freq = HW_SAMPR_DEFAULT;
    unsigned long output = IIS_CONFIG & (7 << 8);

    /* Playback must prevent pops and record monitoring won't work at all if
       adding IIS_RESET when setting IIS_CONFIG. Use a different method for
       each. */
    if (reset && output != (3 << 8))
    {
        /* Not playback - reset first */
        SET_IIS_CONFIG(IIS_RESET);
        reset = false;
    }

    if (pcm_freq != last_pcm_freq)
    {
        last_pcm_freq = pcm_freq;
        audiohw_set_frequency(freq_ent[FPARM_FSEL]);
        coldfire_set_pllcr_audio_bits(PLLCR_SET_AUDIO_BITS_DEFPARM);
    }

    SET_IIS_CONFIG(IIS_DEFPARM(output) | (reset ? IIS_RESET : 0));
} /* pcm_apply_settings */

/** DMA **/

/****************************************************************************
 ** Playback DMA transfer
 **/

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
void pcm_play_dma_start(const void *addr, size_t size)
{
    logf("pcm_play_dma_start");

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    pcm_playing = true;

    /* Set up DMA transfer  */
    SAR0 = (unsigned long)addr;   /* Source address      */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = size;                  /* Bytes to transfer   */

    /* Enable the FIFO and force one write to it */
    pcm_apply_settings(false);

    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA |
           DMA_SINC | DMA_SSIZE(3) | DMA_START;
} /* pcm_play_dma_start */

/* Stops the DMA transfer and interrupt */
void pcm_play_dma_stop(void)
{
    logf("pcm_play_dma_stop");

    pcm_playing = false;

    DCR0 = 0;
    DSR0 = 1;

    /* Reset the FIFO */
    pcm_apply_settings(false);
} /* pcm_play_dma_stop */

void pcm_init(void)
{
    logf("pcm_init");

    pcm_playing           = false;
    pcm_paused            = false;
    pcm_callback_for_more = NULL;

    MPARK     = 0x81;   /* PARK[1,0]=10 + BCR24BIT                   */
    DIVR0     = 54;     /* DMA0 is mapped into vector 54 in system.c */
    DMAROUTE  = (DMAROUTE & 0xffffff00) | DMA0_REQ_AUDIO_1;
    DMACONFIG = 1;      /* DMA0Req = PDOR3, DMA1Req = PDIR2          */

    /* Reset the audio FIFO */
    SET_IIS_CONFIG(IIS_RESET);

    pcm_set_frequency(HW_FREQ_DEFAULT);

    /* Prevent pops (resets DAC to zero point) */
    SET_IIS_CONFIG(IIS_DEFPARM(3 << 8) | IIS_RESET);

#if defined(HAVE_SPDIF_IN) || defined(HAVE_SPDIF_OUT)
    spdif_init();
#endif

    /* Initialize default register values. */
    audiohw_init();

#if defined(HAVE_UDA1380)
    /* Sleep a while so the power can stabilize (especially a long
       delay is needed for the line out connector). */
    sleep(HZ);
    /* Power on FSDAC and HP amp. */
    audiohw_enable_output(true);
#elif defined(HAVE_TLV320)
    sleep(HZ/4);
#endif

    /* UDA1380: Unmute the master channel
       (DAC should be at zero point now). */
    audiohw_mute(false);

    /* Call pcm_play_dma_stop to initialize everything. */
    pcm_play_dma_stop();

    /* Enable interrupt at level 7, priority 0 */
    ICR6 = (7 << 2);
    IMR &= ~(1 << 14);      /* bit 14 is DMA0 */
} /* pcm_init */

size_t pcm_get_bytes_waiting(void)
{
    return BCR0 & 0xffffff;
} /* pcm_get_bytes_waiting */

/* DMA0 Interrupt is called when the DMA has finished transfering a chunk
   from the caller's buffer */
void DMA0(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA0(void)
{
    int res = DSR0;

    DSR0  = 1;    /* Clear interrupt */
    DCR0 &= ~DMA_EEXT;

    /* Stop on error */
    if ((res & 0x70) == 0)
    {
        pcm_more_callback_type get_more  = pcm_callback_for_more;
        unsigned char         *next_start;
        size_t                 next_size = 0;

        if (get_more)
            get_more(&next_start, &next_size);

        if (next_size > 0)
        {
            SAR0  = (unsigned long)next_start;  /* Source address */
            BCR0  = next_size;                  /* Bytes to transfer */
            DCR0 |= DMA_EEXT;
            return;
        }
        else
        {
            /* Finished playing */
#if 0
            /* int. logfs can trash the display */
            logf("DMA0 No Data:0x%04x", res);
#endif
        }
    }
    else
    {
        logf("DMA Error:0x%04x", res);
    }

    pcm_play_dma_stop();
} /* DMA0 */

/****************************************************************************
 ** Recording DMA transfer
 **/
void pcm_rec_dma_start(void *addr, size_t size)
{
    logf("pcm_rec_dma_start");

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    pcm_recording = true;

    pcm_apply_settings(false);

    /* Start the DMA transfer.. */
#ifdef HAVE_SPDIF_IN
    INTERRUPTCLEAR = 0x03c00000;
#endif

    SAR1          = (unsigned long)&PDIR2; /* Source address        */
    rec_peak_addr = (unsigned long *)addr; /* Start peaking at dest */
    DAR1          = (unsigned long)addr;   /* Destination address   */
    BCR1          = (unsigned long)size;   /* Bytes to transfer     */

    DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA | DMA_DINC |
           DMA_DSIZE(3) | DMA_START;
} /* pcm_dma_start */

void pcm_rec_dma_stop(void)
{
    logf("pcm_rec_dma_stop");

    DSR1 = 1;         /* Clear interrupt */
    DCR1 = 0;

    pcm_recording = false;
} /* pcm_dma_stop */

void pcm_init_recording(void)
{
    logf("pcm_init_recording");

    pcm_recording           = false;
    pcm_callback_more_ready = NULL;

    AUDIOGLOB |= 0x180;   /* IIS1 fifo auto sync = on, PDIR2 auto sync = on */

    DIVR1     = 55;       /* DMA1 is mapped into vector 55 in system.c      */
    DMACONFIG = 1;        /* DMA0Req = PDOR3, DMA1Req = PDIR2               */
    DMAROUTE  = (DMAROUTE & 0xffff00ff) | DMA1_REQ_AUDIO_2;

    pcm_rec_dma_stop();

    ICR7 = (7 << 2);        /* Enable interrupt at level 7, priority 0      */
    IMR &= ~(1 << 15);      /* bit 15 is DMA1                               */
} /* pcm_init_recording */

void pcm_close_recording(void)
{
    logf("pcm_close_recording");

    pcm_rec_dma_stop();

    DMAROUTE &= 0xffff00ff;
    ICR7      = 0x00;      /* Disable interrupt */
    IMR      |= (1 << 15); /* bit 15 is DMA1    */
} /* pcm_close_recording */

/* DMA1 Interrupt is called when the DMA has finished transfering a chunk
   into the caller's buffer */
void DMA1(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA1(void)
{
    int res = DSR1;
    int status = 0;
    pcm_more_callback_type2 more_ready;

    DSR1  = 1;          /* Clear interrupt */
    DCR1 &= ~DMA_EEXT;  /* Disable peripheral request */

    if (res & 0x70)
    {
        status = DMA_REC_ERROR_DMA;
        logf("DMA1 err: 0x%x", res);
    }
#ifdef HAVE_SPDIF_IN
    else if (DATAINCONTROL == 0xc038 &&
        (INTERRUPTSTAT & 0x01c00000)) /* valnogood, symbolerr, parityerr */
    {
        INTERRUPTCLEAR = 0x03c00000;
        status = DMA_REC_ERROR_SPDIF;
        logf("spdif err");
    }
#endif

    more_ready = pcm_callback_more_ready;

    if (more_ready != NULL && more_ready(status) >= 0)
        return;

#if 0
    /* int. logfs can trash the display */
    logf("DMA1 done:%04x %d", res, status);
#endif
    /* Finished recording */
    pcm_rec_dma_stop();
} /* DMA1 */

/* Continue transferring data in */
void pcm_record_more(void *start, size_t size)
{
    rec_peak_addr = (unsigned long *)start; /* Start peaking at dest */
    DAR1          = (unsigned long)start;   /* Destination address */
    BCR1          = (unsigned long)size;    /* Bytes to transfer   */
    DCR1         |= DMA_EEXT;
}

void pcm_mute(bool mute)
{
    audiohw_mute(mute);
    if (mute)
        sleep(HZ/16);
} /* pcm_mute */

void pcm_play_pause_pause(void)
{
    /* Disable DMA peripheral request. */
    DCR0 &= ~DMA_EEXT;
    pcm_apply_settings(true);
} /* pcm_play_pause_pause */

void pcm_play_pause_unpause(void)
{
    /* Enable the FIFO and force one write to it */
    pcm_apply_settings(false);
    DCR0 |= DMA_EEXT | DMA_START;
} /* pcm_play_pause_unpause */

/**
 * Do peak calculation using distance squared from axis and save a lot
 * of jumps and negation. Don't bother with the calculations of left or
 * right only as it's never really used and won't save much time.
 */
static void pcm_peak_peeker(unsigned long *addr, unsigned long *end,
                            int peaks[2])
{
    long peak_l   = 0, peak_r   = 0;
    long peaksq_l = 0, peaksq_r = 0;

    do
    {
        long value = *addr;
        long ch, chsq;

        ch   = value >> 16;
        chsq = ch*ch;
        if (chsq > peaksq_l)
            peak_l = ch, peaksq_l = chsq;

        ch   = (short)value;
        chsq = ch*ch;
        if (chsq > peaksq_r)
            peak_r = ch, peaksq_r = chsq;

        addr += 4;
    }
    while (addr < end);

    peaks[0] = abs(peak_l);
    peaks[1] = abs(peak_r);
} /* pcm_peak_peeker */

/**
 * Return playback peaks - Peaks ahead in the DMA buffer based upon the
 *                         calling period to attempt to compensate for
 *                         delay.
 */
void pcm_calculate_peaks(int *left, int *right)
{
    static unsigned long last_peak_tick = 0;
    static unsigned long frame_period   = 0;

    long samples, samp_frames;
    unsigned long *addr;

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
        /* Snapshot as quickly as possible */
        asm volatile (
            "move.l %c[sar0], %[start] \n"
            "move.l %c[bcr0], %[count] \n"
            : [start]"=r"(addr), [count]"=r"(samples)
            : [sar0]"p"(&SAR0), [bcr0]"p"(&BCR0)
        );

        samples    &= 0xfffffc;
        samp_frames = frame_period*pcm_freq/(HZ/4);
        samples     = MIN(samp_frames, samples) >> 2;

        if (samples > 0)
        {
            addr = (long *)((long)addr & ~3);
            pcm_peak_peeker(addr, addr + samples, &peaks[PLAY_PEAK_LEFT]);
        }
    }
    else
    {
        peaks[PLAY_PEAK_LEFT] = peaks[PLAY_PEAK_RIGHT] = 0;
    }

    if (left)
        *left = peaks[PLAY_PEAK_LEFT];

    if (right)
        *right = peaks[PLAY_PEAK_RIGHT];
} /* pcm_calculate_peaks */

/**
 * Return recording peaks - From the end of the last peak up to
 *                          current write position.
 */
void pcm_calculate_rec_peaks(int *left, int *right)
{
    if (pcm_recording)
    {
        unsigned long *addr, *end;

        /* Snapshot as quickly as possible */
        asm volatile (
            "move.l %c[start], %[addr] \n"
            "move.l %c[dar1],  %[end]  \n"
            "and.l  %[mask],   %[addr] \n"
            "and.l  %[mask],   %[end]  \n"
            : [addr]"=r"(addr), [end]"=r"(end)
            : [start]"p"(&rec_peak_addr), [dar1]"p"(&DAR1), [mask]"r"(~3)
        );

        if (addr < end)
        {
            pcm_peak_peeker(addr, end, &peaks[REC_PEAK_LEFT]);

            if (addr == rec_peak_addr)
                rec_peak_addr = end;
        }
    }
    else
    {
        peaks[REC_PEAK_LEFT] = peaks[REC_PEAK_RIGHT] = 0;
    }

    if (left)
        *left = peaks[REC_PEAK_LEFT];

    if (right)
        *right = peaks[REC_PEAK_RIGHT];
} /* pcm_calculate_rec_peaks */
