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

#define IIS_PLAY_DEFPARM    ( (freq_ent[FPARM_CLOCKSEL] << 12) | \
                              (IIS_PLAY & (7 << 8)) | \
                              (4 << 2) )  /* 64 bit clocks / word clock */
#define IIS_FIFO_RESET      (1 << 11)
#define PDIR2_FIFO_RESET    (1 << 9)

#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define SET_IIS_PLAY(x) IIS1CONFIG = (x)
#define IIS_PLAY        IIS1CONFIG
#else
#define SET_IIS_PLAY(x) IIS2CONFIG = (x)
#define IIS_PLAY        IIS2CONFIG
#endif

static bool is_playback_monitoring(void)
{
    return (IIS_PLAY & (7 << 8)) == (3 << 8);
}

static void iis_play_reset_if_playback(bool if_playback)
{
    bool is_playback = is_playback_monitoring();

    if (is_playback != if_playback)
        return;

    or_l(IIS_FIFO_RESET, &IIS_PLAY);
}

#define PLLCR_SET_AUDIO_BITS_DEFPARM \
            ((freq_ent[FPARM_CLSEL] << 28) | (1 << 22))

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
bool _pcm_apply_settings(bool clear_reset)
{
    static int last_pcm_freq = 0;
    bool did_reset = false;
 
    if (pcm_freq != last_pcm_freq)
    {
        last_pcm_freq = pcm_freq;
        /* Reprogramming bits 15-12 requires FIFO to be in a reset
           condition - Users Manual 17-8, Note 11 */
        or_l(IIS_FIFO_RESET, &IIS_PLAY);
        audiohw_set_frequency(freq_ent[FPARM_FSEL]);
        coldfire_set_pllcr_audio_bits(PLLCR_SET_AUDIO_BITS_DEFPARM);
        did_reset = true;
    }

    SET_IIS_PLAY(IIS_PLAY_DEFPARM |
        (clear_reset ? 0 : (IIS_PLAY & IIS_FIFO_RESET)));
#if 0
    logf("IISPLAY: %08X", IIS_PLAY);
#endif

    return did_reset;
} /* _pcm_apply_settings */

/* This clears the reset bit to enable monitoring immediately if monitoring
   recording sources or always if playback is in progress - we might be 
   switching samplerates on the fly */
void pcm_apply_settings(void)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);
    bool pbm = is_playback_monitoring();
    bool kick = (DCR0 & DMA_EEXT) != 0 && pbm;

    /* Clear reset if not playback monitoring or peripheral request is
       active and playback monitoring */
    if (_pcm_apply_settings(!pbm || kick) && kick)
        PDOR3 = 0; /* Kick FIFO out of reset by writing to it */

    set_irq_level(level);
} /* pcm_apply_settings */

/** DMA **/

/****************************************************************************
 ** Playback DMA transfer
 **/

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
void pcm_play_dma_start(const void *addr, size_t size)
{
    int level;

    logf("pcm_play_dma_start");

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* If a tranfer is going, prevent an interrupt while setting up
       a new one */
    level = set_irq_level(DMA_IRQ_LEVEL);

    pcm_playing = true;

    /* Set up DMA transfer  */
    SAR0 = (unsigned long)addr;   /* Source address      */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = (unsigned long)size;   /* Bytes to transfer   */

    /* Enable the FIFO and force one write to it */
    _pcm_apply_settings(is_playback_monitoring());

    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA |
           DMA_SINC | DMA_SSIZE(DMA_SIZE_LINE) | DMA_START;

    set_irq_level(level);
} /* pcm_play_dma_start */

/* Stops the DMA transfer and interrupt */
static void pcm_play_dma_stop_irq(void)
{
    pcm_playing = false;

    DSR0 = 1;
    DCR0 = 0;

    /* Place TX FIFO in reset condition if playback monitoring is on.
       Recording monitoring something else should not be stopped. */
    iis_play_reset_if_playback(true);

    pcm_playing = false;    
} /* pcm_play_dma_stop_irq */

void pcm_play_dma_stop(void)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);

    logf("pcm_play_dma_stop");

    pcm_play_dma_stop_irq();

    set_irq_level(level);
} /* pcm_play_dma_stop */

void pcm_init(void)
{
    logf("pcm_init");

    pcm_playing           = false;
    pcm_paused            = false;
    pcm_callback_for_more = NULL;

    AUDIOGLOB =   (1 <<  8) /* IIS1 fifo auto sync  */
                | (1 <<  7) /* PDIR2 fifo auto sync */
#ifdef HAVE_SPDIF_OUT
                | (1 << 10) /* EBU TX auto sync     */
#endif
                ;
    DIVR0     = 54;     /* DMA0 is mapped into vector 54 in system.c */
    and_l(0xffffff00, &DMAROUTE);
    or_l(DMA0_REQ_AUDIO_1, &DMAROUTE);
    DMACONFIG = 1;      /* DMA0Req = PDOR3, DMA1Req = PDIR2          */

    /* Call pcm_play_dma_stop to initialize everything. */
    pcm_play_dma_stop();
    /* Call pcm_close_recording to put in closed state */
    pcm_close_recording();

    /* Initialize default register values. */
    audiohw_init();

    audio_set_output_source(AUDIO_SRC_PLAYBACK);
    audio_set_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    pcm_set_frequency(HW_FREQ_DEFAULT);

    _pcm_apply_settings(false);

#if defined(HAVE_SPDIF_IN) || defined(HAVE_SPDIF_OUT)
    spdif_init();
#endif
    /* Enable interrupt at level 6, priority 0 */
    ICR6 = (6 << 2);
    and_l(~(1 << 14), &IMR); /* bit 14 is DMA0 */
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

    DSR0  = 1;                  /* Clear interrupt */
    and_l(~DMA_EEXT, &DCR0);    /* Disable peripheral request */
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
            or_l(DMA_EEXT, &DCR0);              /* Enable peripheral request */
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
        logf("DMA0 err: %02x", res);
#if 0
        logf("  SAR0: %08x", SAR0);
        logf("  DAR0: %08x", DAR0);
        logf("  BCR0: %08x", BCR0);
        logf("  DCR0: %08x", DCR0);
#endif
    }

    pcm_play_dma_stop_irq();
} /* DMA0 */

/****************************************************************************
 ** Recording DMA transfer
 **/
void pcm_rec_dma_start(void *addr, size_t size)
{
    int level;
    logf("pcm_rec_dma_start");

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    /* No DMA1 interrupts while setting up a new transfer */
    level = set_irq_level(DMA_IRQ_LEVEL);

    pcm_recording = true;

    and_l(~PDIR2_FIFO_RESET, &DATAINCONTROL);
    /* Clear TX FIFO reset bit if the source is not set to monitor playback
       otherwise maintain independence between playback and recording. */
    _pcm_apply_settings(!is_playback_monitoring());

    /* Start the DMA transfer.. */
#ifdef HAVE_SPDIF_IN
    INTERRUPTCLEAR = 0x03c00000;
#endif

    SAR1          = (unsigned long)&PDIR2; /* Source address        */
    rec_peak_addr = (unsigned long *)addr; /* Start peaking at dest */
    DAR1          = (unsigned long)addr;   /* Destination address   */
    BCR1          = (unsigned long)size;   /* Bytes to transfer     */

    DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA | DMA_DINC |
           DMA_DSIZE(DMA_SIZE_LINE) /* | DMA_START */;

    set_irq_level(level);
} /* pcm_dma_start */

static void pcm_rec_dma_stop_irq(void)
{
    DSR1 = 1;         /* Clear interrupt */
    DCR1 = 0;
    pcm_recording = false;
    or_l(PDIR2_FIFO_RESET, &DATAINCONTROL);

    iis_play_reset_if_playback(false);
} /* pcm_rec_dma_stop_irq */

void pcm_rec_dma_stop(void)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);

    logf("pcm_rec_dma_stop");

    pcm_rec_dma_stop_irq();

    set_irq_level(level);
} /* pcm_rec_dma_stop */

void pcm_init_recording(void)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);

    logf("pcm_init_recording");

    pcm_recording           = false;
    pcm_callback_more_ready = NULL;

    DIVR1     = 55;       /* DMA1 is mapped into vector 55 in system.c     */
    DMACONFIG = 1;        /* DMA0Req = PDOR3, DMA1Req = PDIR2              */
    and_l(0xffff00ff, &DMAROUTE);
    or_l(DMA1_REQ_AUDIO_2, &DMAROUTE);

    pcm_rec_dma_stop_irq();

    ICR7 = (6 << 2) | (1 << 0); /* Enable interrupt at level 6, priority 1 */
    and_l(~(1 << 15), &IMR); /* bit 15 is DMA1                             */

    set_irq_level(level);
} /* pcm_init_recording */

void pcm_close_recording(void)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);

    logf("pcm_close_recording");

    pcm_rec_dma_stop_irq();

    and_l(0xffff00ff, &DMAROUTE);
    ICR7 = 0x00;            /* Disable interrupt */
    or_l((1 << 15), &IMR);  /* bit 15 is DMA1    */

    set_irq_level(level);
} /* pcm_close_recording */

/* DMA1 Interrupt is called when the DMA has finished transfering a chunk
   into the caller's buffer */
void DMA1(void) __attribute__ ((interrupt_handler, section(".icode")));
void DMA1(void)
{
    int res = DSR1;
    int status = 0;
    pcm_more_callback_type2 more_ready;

    DSR1  = 1;               /* Clear interrupt */
    and_l(~DMA_EEXT, &DCR1); /* Disable peripheral request */

    if (res & 0x70)
    {
        status = DMA_REC_ERROR_DMA;
        logf("DMA1 err: %02x", res);
#if 0
        logf("  SAR1: %08x", SAR1);
        logf("  DAR1: %08x", DAR1);
        logf("  BCR1: %08x", BCR1);
        logf("  DCR1: %08x", DCR1);
#endif
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
    pcm_rec_dma_stop_irq();
} /* DMA1 */

/* Continue transferring data in - call from interrupt callback */
void pcm_record_more(void *start, size_t size)
{
    rec_peak_addr = (unsigned long *)start; /* Start peaking at dest */
    DAR1          = (unsigned long)start;   /* Destination address */
    BCR1          = (unsigned long)size;    /* Bytes to transfer   */
    or_l(DMA_EEXT, &DCR1);                  /* Enable peripheral request */
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
    int level = set_irq_level(DMA_IRQ_LEVEL);

    and_l(~DMA_EEXT, &DCR0);
    iis_play_reset_if_playback(true);

    set_irq_level(level);
} /* pcm_play_pause_pause */

void pcm_play_pause_unpause(void)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);

    /* Enable the FIFO and force one write to it */
    _pcm_apply_settings(is_playback_monitoring());
    or_l(DMA_EEXT | DMA_START, &DCR0);

    set_irq_level(level);
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
