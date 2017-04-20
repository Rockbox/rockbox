/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008-2009 Rafaël Carré
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
#include "as3525.h"
#include "pl081.h"
#include "dma-target.h"
#include "clock-target.h"
#include "panic.h"
#include "as3514.h"
#include "audiohw.h"
#include "mmu-arm.h"
#include "pcm-internal.h"

#define MAX_TRANSFER ((1<<11)-1)   /* maximum data we can transfer via DMA
                                    * i.e. 32 bits at once (size of I2SO_DATA)
                                    * and the number of 32bits words has to
                                    * fit in 11 bits of DMA register */

static const void *dma_start_addr;     /* Pointer to callback buffer */
static unsigned long dma_start_frames; /* Count of callback buffer */
static const uint32_t *dma_sub_addr;   /* Pointer to sub buffer */
static unsigned long dma_rem_frames;   /* Remaining frames */
static unsigned long play_sub_frames;  /* frame count of current subtransfer */
static void dma_callback(void);
static unsigned long dma_play_state = 0;

#define DMA_INT_LOCKED  0x1ul
#define DMA_INT_ON      0x2ul
#define DMA_CB_PENDING  0x4ul
#define DMA_PLAYING     0x8ul

/* Mask the DMA interrupt */
void pcm_play_dma_lock(void)
{
    bitset32(&dma_play_state, DMA_INT_LOCKED);
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_dma_unlock(void)
{
    unsigned long state = bitclr32(&dma_play_state,
                                   DMA_INT_LOCKED|DMA_CB_PENDING);

    if ((state & (DMA_INT_ON|DMA_CB_PENDING)) == (DMA_INT_ON|DMA_CB_PENDING))
        dma_callback();
}

static void play_start_pcm(void)
{
    const void *addr = dma_sub_addr;
    unsigned long frames = dma_rem_frames;

    if (frames > MAX_TRANSFER)
        frames = MAX_TRANSFER;

    play_sub_frames = frames;

    dma_enable_channel(0, (void*)addr, (void*)I2SOUT_DATA, DMA_PERI_I2SOUT,
                DMAC_FLOWCTRL_DMAC_MEM_TO_PERI, true, false, frames,
                DMA_S16, dma_callback);
}

static void dma_callback(void)
{
    dma_sub_addr += play_sub_frames;
    dma_rem_frames -= play_sub_frames;
    play_sub_frames = 0; /* Might get called again if locked */

    if (dma_play_state & DMA_INT_LOCKED)
    {
        if (dma_play_state & DMA_PLAYING)
            dma_play_state |= DMA_CB_PENDING;

        return;
    }

    if (dma_rem_frames)
        play_start_pcm();
    else
        pcm_play_dma_complete_callback(0);
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    dma_start_addr = addr;
    dma_start_frames = frames;
    dma_sub_addr = addr;
    dma_rem_frames = frames;
    commit_dcache_range(addr, frames*4);
    play_start_pcm();

    if (dma_play_state & DMA_INT_ON)
        return;

    bitset32(&dma_play_state, DMA_INT_ON);
}

void pcm_play_dma_prepare(void)
{
    bitset32(&dma_play_state, DMA_PLAYING);
    dma_retain();
}

void pcm_play_dma_stop(void)
{
    bitclr32(&dma_play_state, DMA_INT_ON|DMA_PLAYING);

    dma_disable_channel(0);

    /* Ensure frame counts read back 0 */
    DMAC_CH_SRC_ADDR(0) = 0;
    dma_start_addr = NULL;
    dma_start_frames = 0;
    dma_rem_frames = 0;

    dma_release();

    bitclr32(&dma_play_state, DMA_CB_PENDING);
}

void pcm_play_dma_pause(bool pause)
{
    bitmod32(&dma_play_state,
             pause ? 0 : (DMA_PLAYING|DMA_INT_ON),
             DMA_PLAYING|DMA_INT_ON);

    if (pause)
    {
        dma_pause_channel(0);

        /* if producer's buffer finished, upper layer starts anew */
        if (!dma_rem_frames)
            bitclr32(&dma_play_state, DMA_CB_PENDING);
    }
    else
    {
        if (play_sub_frames)
            dma_resume_channel(0);
        /* else unlock calls the callback if sub buffers remain */
    }
}

void pcm_dma_init(const struct pcm_hw_settings *settings)
{
    bitset32(&CGU_PERI, CGU_I2SOUT_APB_CLOCK_ENABLE);
    I2SOUT_CONTROL = (1<<6) | (1<<3);  /* enable dma, stereo */

    audiohw_init();
    pcm_dma_apply_settings(settings);
}

/* divider is 9 bits but the highest one (for 8kHz) fit in 8 bits */
static const unsigned char divider[SAMPR_NUM_FREQ] = {
    [HW_FREQ_96] = ((AS3525_MCLK_FREQ/128 + SAMPR_96/2) / SAMPR_96) - 1,
    [HW_FREQ_88] = ((AS3525_MCLK_FREQ/128 + SAMPR_88/2) / SAMPR_88) - 1,
    [HW_FREQ_64] = ((AS3525_MCLK_FREQ/128 + SAMPR_64/2) / SAMPR_64) - 1,
    [HW_FREQ_48] = ((AS3525_MCLK_FREQ/128 + SAMPR_48/2) / SAMPR_48) - 1,
    [HW_FREQ_44] = ((AS3525_MCLK_FREQ/128 + SAMPR_44/2) / SAMPR_44) - 1,
    [HW_FREQ_32] = ((AS3525_MCLK_FREQ/128 + SAMPR_32/2) / SAMPR_32) - 1,
    [HW_FREQ_24] = ((AS3525_MCLK_FREQ/128 + SAMPR_24/2) / SAMPR_24) - 1,
    [HW_FREQ_22] = ((AS3525_MCLK_FREQ/128 + SAMPR_22/2) / SAMPR_22) - 1,
    [HW_FREQ_16] = ((AS3525_MCLK_FREQ/128 + SAMPR_16/2) / SAMPR_16) - 1,
    [HW_FREQ_12] = ((AS3525_MCLK_FREQ/128 + SAMPR_12/2) / SAMPR_12) - 1,
    [HW_FREQ_11] = ((AS3525_MCLK_FREQ/128 + SAMPR_11/2) / SAMPR_11) - 1,
    [HW_FREQ_8 ] = ((AS3525_MCLK_FREQ/128 + SAMPR_8 /2) / SAMPR_8 ) - 1,
};

void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    int fsel = settings->fsel;

    bitmod32(&CGU_AUDIO,
             (0<<24) |               /* I2SI_MCLK2PAD_EN = disabled */
             (0<<23) |               /* I2SI_MCLK_EN = disabled */
             (0<<14) |               /* I2SI_MCLK_DIV_SEL = unused */
             (0<<12) |               /* I2SI_MCLK_SEL = clk_main */
             (1<<11) |               /* I2SO_MCLK_EN */
             (divider[fsel] << 2) |  /* I2SO_MCLK_DIV_SEL */
             (AS3525_MCLK_SEL << 0), /* I2SO_MCLK_SEL */
             0x01ffffff);
}

unsigned long pcm_get_dma_frames_waiting(void)
{
    int oldstatus = disable_irq_save();
    unsigned long addr = DMAC_CH_SRC_ADDR(0);
    unsigned long startaddr = (unsigned long)dma_start_addr;
    unsigned long startframes = dma_start_frames;
    restore_interrupt(oldstatus);

    return startframes - (addr - startaddr) / 4;
}

const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    int oldstatus = disable_irq_save();
    unsigned long addr = DMAC_CH_SRC_ADDR(0);
    unsigned long startaddr = (unsigned long)dma_start_addr;
    unsigned long startframes = dma_start_frames;
    restore_irq(oldstatus);

    *frames_rem = startframes - (addr - startaddr) / 4;
    return (void *)addr;
}

/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING

/* Stopping playback gates clock if not recording */
static bool rec_inton = false;
static uint32_t *rec_dma_addr, *rec_dma_buf;
static unsigned long rec_dma_frames;
static int keep_sample = 0; /* In nonzero, keep the sample; else, discard it */

void pcm_rec_dma_lock(void)
{
    int oldlevel = disable_irq_save();

    bitset32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE);
    VIC_INT_EN_CLEAR = INTERRUPT_I2SIN;
    I2SIN_MASK = 0; /* disables all interrupts */

    restore_irq(oldlevel);
}


void pcm_rec_dma_unlock(void)
{
    int oldlevel = disable_irq_save();

    if (rec_inton)
    {
        VIC_INT_ENABLE = INTERRUPT_I2SIN;
        I2SIN_MASK = (1<<2); /* I2SIN_MASK_POAF */
    }

    restore_irq(oldlevel);
}


void INT_I2SIN(void)
{
#if CONFIG_CPU == AS3525
    if (audio_channels == 1)
    {
        /* RX is left-channel-only mono */
        while (rec_dma_frames)
        {
            if (I2SIN_RAW_STATUS & (1<<5))
                return; /* empty */

            uint32_t value = *I2SIN_DATA;

            /* Discard every other sample since ADC clock is 1/2 LRCK */
            keep_sample ^= 1;

            if (keep_sample)
            {
                /* Data is in left channel only - copy to right channel
                   14-bit => 16-bit samples */
                value = (uint16_t)(value << 2) | (value << 18);

                if (audio_output_source != AUDIO_SRC_PLAYBACK &&
                    !(dma_play_state & DMA_PLAYING))
                {
                    /* In this case, loopback is manual so that both output
                       channels have audio */
                    if (I2SOUT_RAW_STATUS & (1<<5))
                    {
                        /* Sync output fifo so it goes empty not before input is
                           filled */
                        for (unsigned i = 0; i < 4; i++)
                            *I2SOUT_DATA = 0;
                    }

                    *I2SOUT_DATA = value;
                    *I2SOUT_DATA = value;
                }

                *rec_dma_addr++ = value;
                rec_dma_frames--;
            }
        }
    }
    else
#endif /* CONFIG_CPU == AS3525 */
    {
        /* RX is stereo */
        while (rec_dma_frames)
        {
            if (I2SIN_RAW_STATUS & (1<<5))
                return; /* empty */

            uint32_t value = *I2SIN_DATA;

            /* Discard every other sample since ADC clock is 1/2 LRCK */
            keep_sample ^= 1;

            if (keep_sample)
            {
                /* Loopback is in I2S hardware */

                /* 14-bit => 16-bit samples */
                *rec_dma_addr++ = (value << 2) & ~0x00030000;
                rec_dma_frames--;
            }
        }
    }

    /* Inform middle layer */
    pcm_rec_dma_complete_callback(0);
}


void pcm_rec_dma_stop(void)
{
    VIC_INT_EN_CLEAR = INTERRUPT_I2SIN;
    I2SIN_MASK = 0; /* disables all interrupts */
    bitclr32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE);

    rec_dma_buf = NULL;
    rec_dma_addr = NULL;
    rec_dma_frames = 0;
    rec_inton = false;
}


void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    rec_dma_buf = addr;
    rec_dma_addr = addr;
    rec_dma_frames = frames;
    rec_inton = true;
}


void pcm_rec_dma_prepare(void)
{
    keep_sample = 0;

    /* ensure empty FIFO */
    while (!(I2SIN_RAW_STATUS & (1<<5)))
        *I2SIN_DATA;

    I2SIN_CLEAR = (1<<6) | (1<<0); /* push error, pop error */
}


void pcm_rec_dma_close(void)
{
    bitset32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE);
    pcm_rec_dma_stop();
}


void pcm_rec_dma_init(void)
{
    bitset32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE);

    I2SIN_MASK = 0; /* disables all interrupts */

    /* 14 bits samples, i2c clk src = I2SOUTIF, sdata src = AFE,
     * data valid at positive edge of SCLK */
    I2SIN_CONTROL = (1<<5) | (1<<2);
}


const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    int oldstatus = disable_irq_save();
    unsigned long addr = (unsigned long)rec_dma_addr;
    unsigned long buf = (unsigned long)rec_dma_buf;
    restore_irq(oldstatus);

    *frames_avail = (addr - buf) / 4;
    return (void *)buf;
}

#endif /* HAVE_RECORDING */
