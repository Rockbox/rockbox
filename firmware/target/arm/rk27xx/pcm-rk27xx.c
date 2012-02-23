/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "audio.h"
#include "string.h"
#include "panic.h"
#include "audiohw.h"
#include "sound.h"
#include "pcm-internal.h"

static int locked = 0;

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (++locked == 1)
    {
        int old = disable_irq_save();
        INTC_IMR &= ~(1<<12); /* mask HDMA interrupt */ 
        restore_irq(old);
    }
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if(--locked == 0)
    {
        int old = disable_irq_save();
        INTC_IMR |= (1<<12); /* unmask HDMA interrupt */
        restore_irq(old);
    }
}

void pcm_play_dma_stop(void)
{
    HDMA_CON0 = 0x00;
    HDMA_ISR = 0x00;

    locked = 1;
}

static void hdma_i2s_transfer(const void *addr, size_t size)
{
    SCU_CLKCFG &= ~(1<<3); /* enable HDMA clock */

    commit_discard_dcache_range(addr, size);

    HDMA_ISRC0 = (uint32_t)addr;               /* source address */
    HDMA_IDST0 = (uint32_t)&I2S_TXR;           /* i2s tx fifo */
    HDMA_ICNT0 = (uint16_t)((size>>2) - 1);    /* number of dma transactions
                                                * of transfer size bytes
                                                * (zero based)
                                                */

    HDMA_ISR = ((1<<13) |    /* mask ch1 accumulation overflow irq */
                (1<<12) |    /* mask ch0 accumulation overflow irq */
                (1<<11) |    /* mask ch1 page count down irq */
                (0<<10) |    /* UNMASK ch0 page count down irq */
                 (1<<9) |    /* mask ch0 transfer irq */
                 (1<<8) |    /* mask ch1 transfer irq */
                 (0<<5) |    /* clear ch1 accumulation overflow flag */
                 (0<<4) |    /* clear ch0 accumulation overflow flag */
                 (0<<3) |    /* clear ch1 count down to zero flag */
                 (0<<2) |    /* clear ch0 count down to zero flag */
                 (0<<1) |    /* clear ch1 active flag */
                 (0<<0));    /* clear ch0 active flag */

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

void pcm_play_dma_start(const void *addr, size_t size)
{
    /* Stop any DMA in progress */
    pcm_play_dma_stop();

    /* kick in DMA transfer */
    hdma_i2s_transfer(addr, size);
}

/* pause DMA transfer by disabling clock to DMA module */
void pcm_play_dma_pause(bool pause)
{
    if(pause)
    {
        SCU_CLKCFG |= (1<<3);
        locked = 1;
    }
    else
    {
        SCU_CLKCFG &= ~(1<<3);
        locked = 0;
    }
}

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
    SCU_CLKCFG &= ~((1<<17) | /* i2s_pclk */
                    (1<<16)); /* i2s_clk */
    
    /* configure I2S module */
    I2S_IER = 0; /* disable all i2s interrupts */

    I2S_TXCTL = (1<<16) |  /* LRCK/SCLK = 64 */
                (4<<8)  |  /* MCLK/SCLK = 4 */
                (1<<4)  |  /* 16bit samples */
                (0<<3)  |  /* stereo */
                (0<<1)  |  /* I2S IF */
#ifdef CODEC_SLAVE
                (1<<0);    /* master mode */
#else
                (0<<0);    /* slave mode */
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
	[HW_FREQ_96] = {24, 255, 4, 1},
	[HW_FREQ_48] = {24, 127, 4, 1},
	[HW_FREQ_44] = {24, 293, 4, 4},
	[HW_FREQ_32] = {24, 127, 4, 2},
	[HW_FREQ_24] = {24, 127, 4, 3},
	[HW_FREQ_22] = {24, 146, 4, 4},
	[HW_FREQ_16] = {24, 127, 5, 4},
	[HW_FREQ_12] = {24, 127, 4, 7},
	[HW_FREQ_11] = {24, 146, 4, 9},
	[HW_FREQ_8]  = {24, 127, 5, 9},
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

void pcm_play_dma_init(void)
{
    /* unmask HDMA interrupt in INTC */
    INTC_IMR |= (1<<12);
    INTC_IECR |= (1<<12);

    audiohw_preinit();
    
    i2s_init();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

void pcm_dma_apply_settings(void)
{
#ifdef CODEC_SLAVE
    set_codec_freq(pcm_fsel);
#endif

    audiohw_set_frequency(pcm_fsel);
}

size_t pcm_get_bytes_waiting(void)
{
    /* current terminate count is in transfer size units (4bytes here) */
    return (HDMA_CCNT0 & 0xffff)<<2;
}

/* audio DMA ISR called when chunk from callers buffer has been transfered */
void INT_HDMA(void)
{
    const void *start;
    size_t size;

    if (pcm_play_dma_complete_callback(PCM_DMAST_OK, &start, &size))
    {
        hdma_i2s_transfer(start, size);
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    }
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uint32_t addr;
    
    int old = disable_irq_save();
    addr = HDMA_CSRC0;
    *count = ((HDMA_CCNT0 & 0xffff)<<2);
    restore_interrupt(old);

    return (void*)addr;
}

/****************************************************************************
 ** Recording DMA transfer
 **/
/* TODO */
