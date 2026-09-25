/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 Marcin Bukat
 * Copyright (C) 2011 Andrew Ryabinin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "system.h"
#include "kernel.h"
#include "audio.h"
#include "string.h"
#include "panic.h"
#include "audiohw.h"
#include "sound.h"
#include "pcm-internal.h"
#include "pcm_sink.h"

static int locked = 0;

/* HDMA_ISR holds the interrupt masks (bits 8-13) and flags (bits 0-5) of
 * both channels - channel 0 plays, channel 1 records - so each channel
 * updates only its own bits. A flag is cleared by writing 0. */
#define HDMA_CH0_ISR_BITS   ((1<<12) | (1<<10) | (1<<9) | \
                             (1<<4) | (1<<2) | (1<<0))
#define HDMA_CH1_ISR_BITS   ((1<<13) | (1<<11) | (1<<8) | \
                             (1<<5) | (1<<3) | (1<<1))
/* running: only the page count down interrupt unmasked, flags clear */
#define HDMA_CH0_ISR_RUN    ((1<<12) | (1<<9))
#define HDMA_CH1_ISR_RUN    ((1<<13) | (1<<8))
/* stopped: all masked, flags clear */
#define HDMA_CH0_ISR_STOP   ((1<<12) | (1<<10) | (1<<9))
#define HDMA_CH1_ISR_STOP   ((1<<13) | (1<<11) | (1<<8))
/* count down to zero */
#define HDMA_CH0_DONE       (1<<2)
#define HDMA_CH1_DONE       (1<<3)

static void hdma_isr_update(unsigned long bits, unsigned long val)
{
    int old = disable_irq_save();
    HDMA_ISR = (HDMA_ISR & ~bits) | val;
    restore_irq(old);
}

/* Mask the DMA interrupt */
static void sink_lock(void)
{
    if (++locked == 1)
    {
        int old = disable_irq_save();
        INTC_IMR &= ~IRQ_ARM_HDMA; /* mask HDMA interrupt */ 
        restore_irq(old);
    }
}

/* Unmask the DMA interrupt if enabled */
static void sink_unlock(void)
{
    if(--locked == 0)
    {
        int old = disable_irq_save();
        INTC_IMR |= IRQ_ARM_HDMA; /* unmask HDMA interrupt */
        restore_irq(old);
    }
}

static void sink_dma_stop(void)
{
    HDMA_CON0 = 0x00;
    hdma_isr_update(HDMA_CH0_ISR_BITS, HDMA_CH0_ISR_STOP);

    locked = 1;
}

static void hdma_i2s_transfer(const void *addr, size_t size)
{
    SCU_CLKCFG &= ~CLKCFG_HDMA; /* enable HDMA clock */

    commit_discard_dcache_range(addr, size);

    HDMA_ISRC0 = (uint32_t)addr;               /* source address */
    HDMA_IDST0 = (uint32_t)&I2S_TXR;           /* i2s tx fifo */
    HDMA_ICNT0 = (uint16_t)((size>>2) - 1);    /* number of dma transactions
                                                * of transfer size bytes
                                                * (zero based)
                                                */

    /* mask ch0 accumulation overflow and transfer irqs, unmask its page
     * count down irq, clear its flags */
    hdma_isr_update(HDMA_CH0_ISR_BITS, HDMA_CH0_ISR_RUN);

    HDMA_ISCNT0 = 0x07;      /* slice size in transfer size units (zero base) */

    HDMA_IPNCNTD0 = 0x01;    /* page count */

    HDMA_CON0 = ((0<<23) |   /* page mode */
                 (1<<22) |   /* slice mode */
                 (1<<21) |   /* DMA enable */
                 (1<<18) |   /* generate interrupt */
                 (0<<16) |   /* on-the-fly is not supported by rk27xx */
                 (5<<13) |   /* transfer mode inc8 */
                  (6<<9) |   /* external hdreq from i2s tx */
                  (0<<7) |   /* increment source address */
                  (1<<5) |   /* fixed destination address */
                  (2<<3) |   /* transfer size = 32bits word */
                  (0<<1) |   /* command of software DMA (not relevant) */
                  (1<<0));   /* hardware trigger DMA mode */
}

static void sink_dma_start(const void *addr, size_t size)
{
    /* Stop any DMA in progress */
    sink_dma_stop();

    /* kick in DMA transfer */
    hdma_i2s_transfer(addr, size);
}

#define I2S_FORMAT  ((1<<16) |  /* LRCK/SCLK = 64 */ \
                     (4<<8)  |  /* MCLK/SCLK = 4 */ \
                     (1<<4)  |  /* 16bit samples */ \
                     (0<<3)  |  /* stereo */ \
                     (0<<1)  |  /* I2S IF */ \
                     I2S_MASTER)
#ifdef CODEC_SLAVE
#define I2S_MASTER  (1<<0)     /* master mode */
#else
#define I2S_MASTER  (0<<0)     /* slave mode */
#endif

static void i2s_init(void)
{
#if defined(HAVE_RK27XX_CODEC)
    /* iomux I2S internal */
    SCU_IOMUXA_CON &= ~(1<<19);  /* i2s external bit */
    SCU_IOMUXB_CON &= ~((1<<4) | /* i2s_mclk */
                        (1<<3) | /* i2s_sdo */
                        (1<<2) | /* i2s_sdi */
                        (1<<1) | /* i2s_lrck */
                        (1<<0)); /* i2s_bck */
#else
    /* iomux I2S external */
    SCU_IOMUXA_CON |= (1<<19);   /* i2s external bit */
    SCU_IOMUXB_CON |= ((1<<4) |  /* i2s_mclk */
                       (1<<3) |  /* i2s_sdo */
                       (1<<2) |  /* i2s_sdi */
                       (1<<1) |  /* i2s_lrck */
                       (1<<0));  /* i2s_bck */
#endif

    /* enable i2s clocks */
    SCU_CLKCFG &= ~(CLKCFG_PCLK_I2S | CLKCFG_I2S);
    
    /* configure I2S module */
    I2S_IER = 0; /* disable all i2s interrupts */

    I2S_TXCTL = I2S_FORMAT;
#ifdef HAVE_RECORDING
    /* bit 24 as the original firmware of the Samsung YP-CP3 sets it */
    I2S_RXCTL = I2S_FORMAT | (1<<24);
#endif

    /* the fifo is 16x32bits according to my tests
     * while the docs state 32x32bits
     */
    I2S_FIFOSTS = (1<<18) | /* Tx trigger level half full */
                  (1<<16);  /* Rx trigger level half full */
                  
    I2S_OPR = (1<<17) |  /* reset Tx */
              (1<<16) |  /* reset Rx */
              (0<<6)  |  /* HDMA Req1 enable */
              (1<<5)  |  /* HDMA Req2 disable */
              (0<<4)  |  /* Req1 for Tx fifo */
              (1<<3)  |  /* Req2 for Rx fifo */
              (0<<2)  |  /* normal operation */
#ifdef CODEC_SLAVE
              (1<<1)  |  /* start Tx (master mode) */
              (0<<0);    /* do not start Rx (master mode) */
                         /* setting Rx bit to 1 result in choppy audio */
#else
              (0<<1)  |  /* not used in slave mode */
              (0<<0);    /* not used in slave mode */
#endif
}

#ifdef CODEC_SLAVE
/* When codec is slave we need to setup i2s MCLK clock using codec pll.
 * The MCLK frequency is 256*codec frequency as i2s setup is:
 * LRCK/SCLK = 64 and MCLK/SCLK = 4 (see i2s_init() for reference)
 *
 * PLL output frequency:
 * Fout = ((Fref / (CLKR+1)) * (CLKF+1)) / (CLKOD+1)
 * Fref = 24 MHz
 */
static void set_codec_freq(unsigned int freq)
{
    long timeout;

    /* {CLKR, CLKF, CLKOD, CODECPLL_DIV} */
    static const unsigned int pcm_freq_params[HW_NUM_FREQ][4] = 
    {
        HW_HAVE_96_([HW_FREQ_96] = {24, 255, 4, 1},)
        HW_HAVE_48_([HW_FREQ_48] = {24, 127, 4, 1},)
        HW_HAVE_44_([HW_FREQ_44] = {24, 293, 4, 4},)
        HW_HAVE_32_([HW_FREQ_32] = {24, 127, 4, 2},)
        HW_HAVE_24_([HW_FREQ_24] = {24, 127, 4, 3},)
        HW_HAVE_22_([HW_FREQ_22] = {24, 146, 4, 4},)
        HW_HAVE_16_([HW_FREQ_16] = {24, 127, 5, 4},)
        HW_HAVE_12_([HW_FREQ_12] = {24, 127, 4, 7},)
        HW_HAVE_11_([HW_FREQ_11] = {24, 146, 4, 9},)
        HW_HAVE_8_([HW_FREQ_8]  = {24, 127, 5, 9},)
    };
    /* select divider output from codec pll */
    SCU_DIVCON1 &= ~((1<<9) | (0xF<<5));
    SCU_DIVCON1 |= (pcm_freq_params[freq][3]<<5);

    /* Codec PLL power up */
    SCU_PLLCON3 &= ~(1<<22);

    SCU_PLLCON3 = (1<<24) |    /* Saturation behavior enable */
                  (1<<23) |    /* Enable fast locking circuit */
                  (pcm_freq_params[freq][0]<<16) | /* CLKR factor */
                  (pcm_freq_params[freq][1]<<4)  | /* CLKF factor */
                  (pcm_freq_params[freq][2]<<1) ; /* CLKOD factor */

/* wait for CODEC PLL lock with 10 ms timeout
 * datasheet states that pll lock should take approx. 0.3 ms
 */
    timeout = current_tick + (HZ/100);
    while (!(SCU_STATUS & (1<<2)))
        if (TIME_AFTER(current_tick, timeout))
            break;

}
#endif

static void sink_dma_init(void)
{
    /* unmask HDMA interrupt in INTC */
    INTC_IMR |= IRQ_ARM_HDMA;
    INTC_IECR |= IRQ_ARM_HDMA;

    /* both channels stopped, with the HDMA clocked for the write */
    SCU_CLKCFG &= ~CLKCFG_HDMA;
    HDMA_ISR = HDMA_CH0_ISR_STOP | HDMA_CH1_ISR_STOP;

    audiohw_preinit();
    
    i2s_init();
}

static void sink_set_freq(uint16_t freq)
{
#ifdef CODEC_SLAVE
    set_codec_freq(freq);
#endif

    audiohw_set_frequency(freq);
}

#ifdef HAVE_RECORDING
static void rec_dma_done(void);
static volatile int rec_locked = 0;
static volatile bool rec_pending = false;
#endif

/* audio DMA ISR called when chunk from callers buffer has been transfered */
void INT_HDMA(void)
{
    unsigned long isr = HDMA_ISR;

    if (isr & HDMA_CH0_DONE)
    {
        const void *start;
        size_t size;

        hdma_isr_update(HDMA_CH0_DONE, 0);

        if (pcm_play_dma_complete_callback(PCM_DMAST_OK, &start, &size))
        {
            hdma_i2s_transfer(start, size);
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        }
    }

#ifdef HAVE_RECORDING
    if (isr & HDMA_CH1_DONE)
    {
        hdma_isr_update(HDMA_CH1_DONE, 0);

        if (rec_locked)
        {
            rec_pending = true;
        }
        else
        {
            rec_dma_done();
        }
    }
#endif
}

struct pcm_sink builtin_pcm_sink = {
    .caps = {
        .samprs       = hw_freq_sampr,
        .num_samprs   = HW_NUM_FREQ,
        .default_freq = HW_FREQ_DEFAULT,
        .volume_type  = PCM_NATIVE_VOLUME_TYPE,
    },
    .ops = {
        .init     = sink_dma_init,
        .postinit = audiohw_postinit,
        .set_freq = sink_set_freq,
        .lock     = sink_lock,
        .unlock   = sink_unlock,
        .play     = sink_dma_start,
        .stop     = sink_dma_stop,
    },
};

/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
/* HDMA channel 1 from the I2S Rx FIFO. Playback and recording share the
 * HDMA interrupt, so locking recording cannot mask it; a buffer completed
 * while locked is handed over at unlock instead. */

/* I2S_OPR while playing only, and while recording too. In master mode the
 * Rx side runs only while recording: started with nothing reading its FIFO
 * it upsets playback. */
#ifdef CODEC_SLAVE
#define I2S_OPR_START_TX    (1<<1)
#define I2S_OPR_START_RX    (1<<0)
#else
#define I2S_OPR_START_TX    0      /* not used in slave mode */
#define I2S_OPR_START_RX    0
#endif
#define I2S_OPR_PLAY        ((0<<6) |  /* HDMA Req1 enable */ \
                             (1<<5) |  /* HDMA Req2 disable */ \
                             (0<<4) |  /* Req1 for Tx fifo */ \
                             (1<<3) |  /* Req2 for Rx fifo */ \
                             I2S_OPR_START_TX)
#define I2S_OPR_RECORD      ((I2S_OPR_PLAY & ~(1<<5)) | I2S_OPR_START_RX)

static void hdma_i2s_rec_transfer(void *addr, size_t size)
{
    SCU_CLKCFG &= ~CLKCFG_HDMA; /* enable HDMA clock */

    /* the DMA writes the buffer: no line of it may be written back over
     * the samples later */
    commit_discard_dcache_range(addr, size);

    HDMA_ISRC1 = (uint32_t)&I2S_RXR;           /* i2s rx fifo */
    HDMA_IDST1 = (uint32_t)addr;               /* destination address */
    HDMA_ICNT1 = (uint16_t)((size>>2) - 1);    /* number of dma transactions
                                                * of transfer size bytes
                                                * (zero based)
                                                */

    hdma_isr_update(HDMA_CH1_ISR_BITS, HDMA_CH1_ISR_RUN);

    HDMA_ISCNT1 = 0x07;      /* slice size in transfer size units (zero base) */

    HDMA_IPNCNTD1 = 0x01;    /* page count */

    HDMA_CON1 = ((0<<23) |   /* page mode */
                 (1<<22) |   /* slice mode */
                 (1<<21) |   /* DMA enable */
                 (1<<18) |   /* generate interrupt */
                 (0<<16) |   /* on-the-fly is not supported by rk27xx */
                 (5<<13) |   /* transfer mode inc8 */
                  (7<<9) |   /* external hdreq from i2s rx */
                  (1<<7) |   /* fixed source address */
                  (0<<5) |   /* increment destination address */
                  (2<<3) |   /* transfer size = 32bits word */
                  (0<<1) |   /* command of software DMA (not relevant) */
                  (1<<0));   /* hardware trigger DMA mode */
}

static void rec_dma_done(void)
{
    void *start;
    size_t size;

    if (pcm_rec_dma_complete_callback(PCM_DMAST_OK, &start, &size))
    {
        hdma_i2s_rec_transfer(start, size);
        pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
    }
}

void pcm_rec_lock(void)
{
    int old = disable_irq_save();
    ++rec_locked;
    restore_irq(old);
}

void pcm_rec_unlock(void)
{
    int old = disable_irq_save();

    if (--rec_locked == 0 && rec_pending)
    {
        rec_pending = false;
        rec_dma_done();
    }

    restore_irq(old);
}

void pcm_rec_dma_stop(void)
{
    HDMA_CON1 = 0x00;
    hdma_isr_update(HDMA_CH1_ISR_BITS, HDMA_CH1_ISR_STOP);
    rec_pending = false;

    I2S_OPR = I2S_OPR_PLAY;
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    pcm_rec_dma_stop();

    I2S_OPR = I2S_OPR_PLAY | (1<<16);   /* reset Rx */
    I2S_OPR = I2S_OPR_RECORD;

    hdma_i2s_rec_transfer(addr, size);
}

void pcm_rec_dma_init(void)
{
    pcm_rec_dma_stop();
}

void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
}

const void * pcm_rec_dma_get_peak_buffer(void)
{
    /* where the DMA writes now */
    return (const void *)(HDMA_CDST1 & ~3);
}
#endif /* HAVE_RECORDING */
