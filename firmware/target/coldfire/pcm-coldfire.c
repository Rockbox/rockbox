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

/* Avoid further #ifdef's for some codec functions */
#if defined(HAVE_UDA1380)
#define ac_init             uda1380_init
#define ac_mute             uda1380_mute
#define ac_set_frequency    uda1380_set_frequency
#elif defined(HAVE_TLV320)
#define ac_init             tlv320_init
#define ac_mute             tlv320_mute
#define ac_set_frequency    tlv320_set_frequency
#endif

/** Semi-private shared symbols **/

/* the registered callback function to ask for more pcm data */
extern volatile pcm_more_callback_type pcm_callback_for_more;
extern volatile bool pcm_playing;
extern volatile bool pcm_paused;

/* the registered callback function for when more data is available */
extern volatile pcm_more_callback_type pcm_callback_more_ready;
extern volatile bool pcm_recording;

/* peaks */
static int play_peak_left, play_peak_right;
static unsigned long *rec_peak_addr;
static int rec_peak_left, rec_peak_right;

#define IIS_DEFPARM     ( (freq_ent[FPARM_CLOCKSEL] << 12) | \
                          (pcm_txsrc_select[pcm_monitor+1] << 8) | \
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

/** monitoring/source selection **/
static int pcm_monitor = AUDIO_SRC_PLAYBACK;

static const unsigned char pcm_txsrc_select[AUDIO_NUM_SOURCES+1] =
{
    [AUDIO_SRC_PLAYBACK+1] = 3, /* PDOR3        */
    [AUDIO_SRC_MIC+1]      = 4, /* IIS1 RcvData */
    [AUDIO_SRC_LINEIN+1]   = 4, /* IIS1 RcvData */
#ifdef HAVE_FMRADIO_IN
    [AUDIO_SRC_FMRADIO+1]  = 4, /* IIS1 RcvData */
#endif
#ifdef HAVE_SPDIF_IN
    [AUDIO_SRC_SPDIF+1]    = 7, /* EBU1 RcvData */
#endif
};

static const unsigned short pcm_dataincontrol[AUDIO_NUM_SOURCES+1] =
{
    [AUDIO_SRC_PLAYBACK+1] = 0x0200, /* Reset PDIR2 data flow */
    [AUDIO_SRC_MIC+1]      = 0xc020, /* Int. when 6 samples in FIFO,
                                        PDIR2 src = ebu1RcvData */
    [AUDIO_SRC_LINEIN+1]   = 0xc020, /* Int. when 6 samples in FIFO,
                                        PDIR2 src = ebu1RcvData */
#ifdef HAVE_FMRADIO_IN
    [AUDIO_SRC_FMRADIO+1]  = 0xc020, /* Int. when 6 samples in FIFO,
                                        PDIR2 src = ebu1RcvData */
#endif
#ifdef HAVE_SPDIF_IN
    [AUDIO_SRC_SPDIF+1]    = 0xc038, /* Int. when 6 samples in FIFO,
                                        PDIR2 src = ebu1RcvData */
#endif
};

static int pcm_rec_src = AUDIO_SRC_PLAYBACK;

void pcm_set_monitor(int monitor)
{
    if ((unsigned)monitor >= AUDIO_NUM_SOURCES)
        monitor = AUDIO_SRC_PLAYBACK;
    pcm_monitor = monitor;
} /* pcm_set_monitor */

void pcm_set_rec_source(int source)
{
    if ((unsigned)source >= AUDIO_NUM_SOURCES)
        source = AUDIO_SRC_PLAYBACK;
    pcm_rec_src = source;
} /* pcm_set_rec_source */

/* apply audio settings */
void pcm_apply_settings(bool reset)
{
    static int last_pcm_freq    = HW_SAMPR_DEFAULT;
#if 0
    static int last_pcm_monitor = AUDIO_SRC_PLAYBACK;
#endif
    static int last_pcm_rec_src = AUDIO_SRC_PLAYBACK;

    /* Playback must prevent pops and record monitoring won't work at all
       adding IIS_RESET when setting IIS_CONFIG. Use a different method for
       each. */
    if (reset && (pcm_monitor != AUDIO_SRC_PLAYBACK))
    {
        /* Not playback - reset first */
        SET_IIS_CONFIG(IIS_RESET);
        reset = false;
    }

    if (pcm_rec_src != last_pcm_rec_src)
    {
        last_pcm_rec_src = pcm_rec_src;
        DATAINCONTROL = pcm_dataincontrol[pcm_rec_src+1];
    }

    if (pcm_freq != last_pcm_freq)
    {
        last_pcm_freq = pcm_freq;
        ac_set_frequency(freq_ent[FPARM_FSEL]);
        coldfire_set_pllcr_audio_bits(PLLCR_SET_AUDIO_BITS_DEFPARM);
    }

    SET_IIS_CONFIG(IIS_DEFPARM | (reset ? IIS_RESET : 0));
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

    pcm_set_frequency(-1);
    pcm_set_monitor(-1);

    /* Prevent pops (resets DAC to zero point) */
    SET_IIS_CONFIG(IIS_DEFPARM | IIS_RESET);

#if defined(HAVE_SPDIF_IN) || defined(HAVE_SPDIF_OUT)
    spdif_init();
#endif

    /* Initialize default register values. */
    ac_init();

#if defined(HAVE_UDA1380)
    /* Sleep a while so the power can stabilize (especially a long
       delay is needed for the line out connector). */
    sleep(HZ);
    /* Power on FSDAC and HP amp. */
    uda1380_enable_output(true);
#elif defined(HAVE_TLV320)
    sleep(HZ/4);
#endif

    /* UDA1380: Unmute the master channel
       (DAC should be at zero point now). */
    ac_mute(false);

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
void pcm_rec_dma_start(const void *addr, size_t size)
{
    logf("pcm_rec_dma_start");

    addr = (void *)((unsigned long)addr & ~3); /* Align data */
    size &= ~3; /* Size must be multiple of 4 */

    pcm_recording = true;

    DAR1 = (unsigned long)addr;   /* Destination address */
    SAR1 = (unsigned long)&PDIR2; /* Source address      */
    BCR1 = size;                  /* Bytes to transfer   */

    rec_peak_addr = (unsigned long *)addr;

    pcm_apply_settings(false);

    /* Start the DMA transfer.. */
#ifdef HAVE_SPDIF_IN
    INTERRUPTCLEAR = 0x03c00000;
#endif

    DCR1 = DMA_INT | DMA_EEXT | DMA_CS | DMA_AA | DMA_DINC |
           DMA_DSIZE(3) | DMA_START;
} /* pcm_dma_start */

void pcm_rec_dma_stop(void)
{
    logf("pcm_rec_dma_stop");

    pcm_recording = false;

    DCR1 = 0;
    DSR1 = 1;         /* Clear interrupt */
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
    pcm_more_callback_type more_ready;
    unsigned char         *next_start;
    ssize_t                next_size = 0; /* passing <> 0 is indicates
                                             an error condition */

    DSR1  = 1;    /* Clear interrupt */
    DCR1 &= ~DMA_EEXT;

    if (res & 0x70)
    {
        next_size = DMA_REC_ERROR_DMA;
        logf("DMA1 err: 0x%x", res);
    }
#ifdef HAVE_SPDIF_IN
    else if (pcm_rec_src == AUDIO_SRC_SPDIF &&
        (INTERRUPTSTAT & 0x01c00000)) /* valnogood, symbolerr, parityerr */
    {
        INTERRUPTCLEAR = 0x03c00000;
        next_size = DMA_REC_ERROR_SPDIF;
        logf("spdif err");
    }
#endif

    more_ready = pcm_callback_more_ready;

    if (more_ready)
        more_ready(&next_start, &next_size);

    if (next_size > 0)
    {
        /* Start peaking at dest */
        rec_peak_addr = (unsigned long *)next_start;
        DAR1  = (unsigned long)next_start; /* Destination address */
        BCR1  = (unsigned long)next_size;  /* Bytes to transfer   */
        DCR1 |= DMA_EEXT;
        return;
    }
    else
    {
#if 0
        /* int. logfs can trash the display */
        logf("DMA1 No Data:0x%04x", res);
#endif
    }

    /* Finished recording */
    pcm_rec_dma_stop();
} /* DMA1 */

void pcm_mute(bool mute)
{
    ac_mute(mute);
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
 * Return playback peaks - Peaks ahead in the DMA buffer based upon the
 *                         calling period to attempt to compensate for
 *                         delay.
 */
void pcm_calculate_peaks(int *left, int *right)
{
    unsigned long samples;
    unsigned long *addr, *end;
    long peak_p, peak_n;
    int level;

    static unsigned long last_peak_tick = 0;
    static unsigned long frame_period = 0;

    /* Throttled peak ahead based on calling period */
    unsigned long period = current_tick - last_peak_tick;

    /* Keep reasonable limits on period */
    if (period < 1)
        period = 1;
    else if (period > HZ/5)
        period = HZ/5;

    frame_period = (3*frame_period + period) >> 2;

    last_peak_tick = current_tick;

    if (!pcm_playing || pcm_paused)
    {
        play_peak_left = play_peak_right = 0;
        goto peak_done;
    }

    /* prevent interrupt from setting up next transfer and
       be sure SAR0 and BCR0 refer to current transfer */
    level = set_irq_level(HIGHEST_IRQ_LEVEL);

    addr    = (long *)(SAR0 & ~3);
    samples = (BCR0 & 0xffffff) >> 2;

    set_irq_level(level);

    samples = MIN(frame_period*pcm_freq/HZ, samples);
    end     = addr + samples;
    peak_p  = peak_n = 0;

    if (left && right)
    {
        if (samples > 0)
        {
            long peak_rp = 0, peak_rn = 0;

            do
            {
                long value = *addr;
                long ch;

                ch = value >> 16;
                if (ch > peak_p)       peak_p  = ch;
                else if (ch < peak_n)  peak_n  = ch;

                ch = (short)value;
                if (ch > peak_rp)      peak_rp = ch;
                else if (ch < peak_rn) peak_rn = ch;

                addr += 4;
            }
            while (addr < end);

            play_peak_left  = MAX(peak_p,  -peak_n);
            play_peak_right = MAX(peak_rp, -peak_rn);
        }
    }
    else if (left || right)
    {
        if (samples > 0)
        {
            if (left)
            {
                /* Put left channel in low word */
                addr = (long *)((short *)addr - 1);
                end  = (long *)((short *)end  - 1);
            }

            do
            {
                long value = *(short *)addr;

                if (value > peak_p)      peak_p = value;
                else if (value < peak_n) peak_n = value;

                addr += 4;
            }
            while (addr < end);

            if (left)
                play_peak_left  = MAX(peak_p, -peak_n);
            else
                play_peak_right = MAX(peak_p, -peak_n);
        }
    }

peak_done:
    if (left)
        *left = play_peak_left;

    if (right)
        *right = play_peak_right;
} /* pcm_calculate_peaks */

/**
 * Return recording peaks - Looks at every 4th sample from last peak up to
 *                          current write position.
 */
void pcm_calculate_rec_peaks(int *left, int *right)
{
    unsigned long *pkaddr, *addr, *end;
    long peak_lp, peak_ln; /* L +,- */
    long peak_rp, peak_rn; /* R +,- */
    int level;

    if (!pcm_recording)
    {
        rec_peak_left = rec_peak_right = 0;
        goto peak_done;
    }

    /* read these atomically or each value may not refer to the
       same data transfer */
    level = set_irq_level(HIGHEST_IRQ_LEVEL);

    pkaddr = rec_peak_addr;
    addr   = pkaddr;
    end    = (unsigned long *)(DAR1 & ~3);

    set_irq_level(level);

    if (addr < end)
    {
        peak_lp = peak_ln =
        peak_rp = peak_rn = 0;

        /* peak one sample per line */
        do
        {
            long value = *addr;
            long ch;

            ch = value >> 16;
            if (ch < peak_ln)
                peak_ln = ch;
            else if (ch > peak_lp)
                peak_lp = ch;

            ch = (short)value;
            if (ch > peak_rp)
                peak_rp = ch;
            else if (ch < peak_rn)
                peak_rn = ch;

            addr += 4;
        }
        while (addr < end);

        /* only update rec_peak_addr if a DMA interrupt hasn't already
           done so */
        level = set_irq_level(HIGHEST_IRQ_LEVEL);

        if (pkaddr == rec_peak_addr)
            rec_peak_addr = end;

        set_irq_level(level);

        /* save peaks */
        rec_peak_left  = MAX(peak_lp, -peak_ln);
        rec_peak_right = MAX(peak_rp, -peak_rn);
    }

peak_done:
    if (left)
        *left  = rec_peak_left;

    if (right)
        *right = rec_peak_right;
} /* pcm_calculate_rec_peaks */

/**
 * Select VINL & VINR source: 0=Line-in, 1=FM Radio
 */
/* All use GPIO */
#if defined(IAUDIO_X5)
    #define REC_MUX_BIT         (1 << 29)
    #define REC_MUX_SET_LINE()  or_l(REC_MUX_BIT, &GPIO_OUT)
    #define REC_MUX_SET_FM()    and_l(~REC_MUX_BIT, &GPIO_OUT)
#else
#if defined(IRIVER_H100_SERIES)
    #define REC_MUX_BIT         (1 << 23)
#elif defined(IRIVER_H300_SERIES)
    #define REC_MUX_BIT         (1 << 30)
#endif
    #define REC_MUX_SET_LINE()  and_l(~REC_MUX_BIT, &GPIO_OUT)
    #define REC_MUX_SET_FM()    or_l(REC_MUX_BIT, &GPIO_OUT)
#endif

void pcm_rec_mux(int source)
{
    if (source == 0)
        REC_MUX_SET_LINE();     /* Line In */
    else
        REC_MUX_SET_FM();       /* FM radio */

    or_l(REC_MUX_BIT, &GPIO_ENABLE);
    or_l(REC_MUX_BIT, &GPIO_FUNCTION);
} /* pcm_rec_mux */
