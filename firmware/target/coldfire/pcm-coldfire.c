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
#if defined(HAVE_SPDIF_REC) || defined(HAVE_SPDIF_OUT)
#include "spdif.h"
#endif

#define IIS_PLAY_DEFPARM    ( (freq_ent[FPARM_CLOCKSEL] << 12) | \
                              (IIS_PLAY & (7 << 8)) | \
                              (4 << 2) )  /* 64 bit clocks / word clock */
#define IIS_FIFO_RESET      (1 << 11)
#define PDIR2_FIFO_RESET    (1 << 9)

#if defined(IAUDIO_X5) || defined(IAUDIO_M5) || defined(IAUDIO_M3)
#define SET_IIS_PLAY(x) IIS1CONFIG = (x)
#define IIS_PLAY        IIS1CONFIG
#else
#define SET_IIS_PLAY(x) IIS2CONFIG = (x)
#define IIS_PLAY        IIS2CONFIG
#endif

struct dma_lock
{
    int locked;
    unsigned long state;
};

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

#if (CONFIG_CPU == MCF5250 || CONFIG_CPU == MCF5249) && defined(HAVE_TLV320)
static const unsigned char pcm_freq_parms[HW_NUM_FREQ][3] =
{
    [HW_FREQ_88] = { 0x0c, 0x01, 0x02 },
    [HW_FREQ_44] = { 0x06, 0x01, 0x01 },
    [HW_FREQ_22] = { 0x04, 0x01, 0x00 },
    [HW_FREQ_11] = { 0x02, 0x02, 0x00 },
};
#endif

static unsigned long pcm_freq = 0; /* 44.1 is default */
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
static bool _pcm_apply_settings(bool clear_reset)
{
    bool did_reset = false;
    unsigned long iis_play_defparm = IIS_PLAY_DEFPARM;
 
    if (pcm_freq != pcm_curr_sampr)
    {
        pcm_curr_sampr = pcm_freq;
        /* Reprogramming bits 15-12 requires FIFO to be in a reset
           condition - Users Manual 17-8, Note 11 */
        or_l(IIS_FIFO_RESET, &IIS_PLAY);
        /* Important for TLV320 - this must happen in the correct order
           or starting recording will sound absolutely awful once in
           awhile - audiohw_set_frequency then coldfire_set_pllcr_audio_bits
         */
        SET_IIS_PLAY(iis_play_defparm | IIS_FIFO_RESET);
        audiohw_set_frequency(freq_ent[FPARM_FSEL]);
        coldfire_set_pllcr_audio_bits(PLLCR_SET_AUDIO_BITS_DEFPARM);
        did_reset = true;
    }

    /* If a reset was done because of a sample rate change, IIS_PLAY will have
       been set already, so only needs to be set again if the reset flag must
       be cleared. If the frequency didn't change, it was never altered and
       the reset flag can just be removed or no action taken. */
    if (clear_reset)
        SET_IIS_PLAY(iis_play_defparm & ~IIS_FIFO_RESET);
#if 0
    logf("IISPLAY: %08X", IIS_PLAY);
#endif

    return did_reset;
} /* _pcm_apply_settings */

/* apply audio setting with all DMA interrupts disabled */
static void _pcm_apply_settings_irq_lock(bool clear_reset)
{
    int level = set_irq_level(DMA_IRQ_LEVEL);
    _pcm_apply_settings(clear_reset);
    restore_irq(level);
}

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

    restore_irq(level);
} /* pcm_apply_settings */

void pcm_play_dma_init(void)
{
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

    /* Setup Coldfire I2S before initializing hardware or changing
       other settings. */
    or_l(IIS_FIFO_RESET, &IIS_PLAY);
    SET_IIS_PLAY(IIS_PLAY_DEFPARM | IIS_FIFO_RESET);
    pcm_set_frequency(HW_FREQ_DEFAULT);
    audio_set_output_source(AUDIO_SRC_PLAYBACK);

    /* Initialize default register values. */
    audiohw_init();

    audio_input_mux(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);

    audiohw_set_frequency(freq_ent[FPARM_FSEL]);
    coldfire_set_pllcr_audio_bits(PLLCR_SET_AUDIO_BITS_DEFPARM);

#if defined(HAVE_SPDIF_REC) || defined(HAVE_SPDIF_OUT)
    spdif_init();
#endif
    /* Enable interrupt at level 6, priority 0 */
    ICR6 = (6 << 2);
} /* pcm_play_dma_init */

void pcm_postinit(void)
{
    audiohw_postinit();
}

/** DMA **/

/****************************************************************************
 ** Playback DMA transfer
 **/
/* For the locks, DMA interrupt must be disabled when manipulating the lock
   if the handler ever calls these - right now things are arranged so it
   doesn't */
static struct dma_lock dma_play_lock =
{
    .locked = 0,
    .state = (0 << 14)  /* bit 14 is DMA0 */
};

void pcm_play_lock(void)
{
    if (++dma_play_lock.locked == 1)
        or_l((1 << 14), &IMR);
}

void pcm_play_unlock(void)
{
    if (--dma_play_lock.locked == 0)
        and_l(~dma_play_lock.state, &IMR);
}

/* Set up the DMA transfer that kicks in when the audio FIFO gets empty */
void pcm_play_dma_start(const void *addr, size_t size)
{
    logf("pcm_play_dma_start");

    /* stop any DMA in progress */
    DSR0 = 1;
    DCR0 = 0;

    /* Set up DMA transfer  */
    SAR0 = (unsigned long)addr;   /* Source address      */
    DAR0 = (unsigned long)&PDOR3; /* Destination address */
    BCR0 = (unsigned long)size;   /* Bytes to transfer   */

    /* Enable the FIFO and force one write to it */
    _pcm_apply_settings_irq_lock(is_playback_monitoring());

    DCR0 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA |
           DMA_SINC | DMA_SSIZE(DMA_SIZE_LINE) | DMA_START;

    dma_play_lock.state = (1 << 14);
} /* pcm_play_dma_start */

/* Stops the DMA transfer and interrupt */
void pcm_play_dma_stop(void)
{
    DSR0 = 1;
    DCR0 = 0;
    BCR0 = 0;

    /* Place TX FIFO in reset condition if playback monitoring is on.
       Recording monitoring something else should not be stopped. */
    iis_play_reset_if_playback(true);

    dma_play_lock.state = (0 << 14);
} /* pcm_play_dma_stop */

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        /* pause playback on current buffer */
        and_l(~DMA_EEXT, &DCR0);
        iis_play_reset_if_playback(true);
        dma_play_lock.state = (0 << 14);
    }
    else
    {
        /* restart playback on current buffer */
        /* Enable the FIFO and force one write to it */
        _pcm_apply_settings_irq_lock(is_playback_monitoring());
        or_l(DMA_EEXT | DMA_START, &DCR0);
        dma_play_lock.state = (1 << 14);
    }
} /* pcm_play_dma_pause */

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

    /* Stop interrupt and futher transfers */
    pcm_play_dma_stop();
    /* Inform PCM that we're done */
    pcm_play_dma_stopped_callback();
} /* DMA0 */

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    unsigned long addr = SAR0;
    int cnt = BCR0;
    *count = (cnt & 0xffffff) >> 2;
    return (void *)((addr + 2) & ~3);
} /* pcm_play_dma_get_peak_buffer */

/****************************************************************************
 ** Recording DMA transfer
 **/
static struct dma_lock dma_rec_lock =
{
    .locked = 0,
    .state = (0 << 15)  /* bit 15 is DMA1 */
};

/* For the locks, DMA interrupt must be disabled when manipulating the lock
   if the handler ever calls these - right now things are arranged so it
   doesn't */
void pcm_rec_lock(void)
{
    if (++dma_rec_lock.locked == 1)
        or_l((1 << 15), &IMR);
}

void pcm_rec_unlock(void)
{
    if (--dma_rec_lock.locked == 0)
        and_l(~dma_rec_lock.state, &IMR);
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    /* stop any DMA in progress */
    and_l(~DMA_EEXT, &DCR1);
    DSR1 = 1;

    and_l(~PDIR2_FIFO_RESET, &DATAINCONTROL);
    /* Clear TX FIFO reset bit if the source is not set to monitor playback
       otherwise maintain independence between playback and recording. */
    _pcm_apply_settings_irq_lock(!is_playback_monitoring());

    /* Start the DMA transfer.. */
#ifdef HAVE_SPDIF_REC
    /* clear: ebu1cnew, valnogood, symbolerr, parityerr */
    INTERRUPTCLEAR = (1 << 25) | (1 << 24) | (1 << 23) | (1 << 22);
#endif

    SAR1              = (unsigned long)&PDIR2; /* Source address        */
    pcm_rec_peak_addr = (unsigned long *)addr; /* Start peaking at dest */
    DAR1              = (unsigned long)addr;   /* Destination address   */
    BCR1              = (unsigned long)size;   /* Bytes to transfer     */

    DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA | DMA_DINC |
           DMA_DSIZE(DMA_SIZE_LINE) | DMA_START;

    dma_rec_lock.state = (1 << 15);
} /* pcm_rec_dma_start */

void pcm_rec_dma_stop(void)
{
    DSR1 = 1;         /* Clear interrupt */
    DCR1 = 0;
    BCR1 = 0;
    or_l(PDIR2_FIFO_RESET, &DATAINCONTROL);

    iis_play_reset_if_playback(false);

    dma_rec_lock.state = (0 << 15);
} /* pcm_rec_dma_stop */

void pcm_rec_dma_init(void)
{
    DIVR1     = 55; /* DMA1 is mapped into vector 55 in system.c */
    DMACONFIG = 1;  /* DMA0Req = PDOR3, DMA1Req = PDIR2 */
    and_l(0xffff00ff, &DMAROUTE);
    or_l(DMA1_REQ_AUDIO_2, &DMAROUTE);

    pcm_rec_dma_stop();

    /* Enable interrupt at level 6, priority 1 */
    ICR7 = (6 << 2) | (1 << 0);
} /* pcm_init_recording */

void pcm_rec_dma_close(void)
{
    and_l(0xffff00ff, &DMAROUTE);
    ICR7 = 0x00;     /* Disable interrupt */
    dma_rec_lock.state = (0 << 15);
} /* pcm_rec_dma_close */

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
#ifdef HAVE_SPDIF_REC
    else if (DATAINCONTROL == 0xc038 &&
        (INTERRUPTSTAT & ((1 << 24) | (1 << 23) | (1 << 22))))
    {
        /* reason: valnogood, symbolerr, parityerr */
        /* clear: ebu1cnew, valnogood, symbolerr, parityerr */
        INTERRUPTCLEAR = (1 << 25) | (1 << 24) | (1 << 23) | (1 << 22);
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
    /* Inform PCM that we're done */
    pcm_rec_dma_stopped_callback();
} /* DMA1 */

/* Continue transferring data in - call from interrupt callback */
void pcm_record_more(void *start, size_t size)
{
    pcm_rec_peak_addr = (unsigned long *)start; /* Start peaking at dest */
    DAR1              = (unsigned long)start;   /* Destination address */
    BCR1              = (unsigned long)size;    /* Bytes to transfer   */
    or_l(DMA_EEXT, &DCR1);                  /* Enable peripheral request */
} /* pcm_record_more */

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    unsigned long addr = (unsigned long)pcm_rec_peak_addr;
    unsigned long end = DAR1;
    addr >>= 2;
    *count = (end >> 2) - addr;
    return (void *)(addr << 2);
} /* pcm_rec_dma_get_peak_buffer */
