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

#define MAX_TRANSFER (4*((1<<11)-1)) /* maximum data we can transfer via DMA
                                      * i.e. 32 bits at once (size of I2SO_DATA)
                                      * and the number of 32bits words has to
                                      * fit in 11 bits of DMA register */

static const void *dma_start_addr;    /* Pointer to callback buffer */
static size_t dma_start_size;   /* Size of callback buffer */
static const void *dma_sub_addr;      /* Pointer to sub buffer */
static size_t dma_rem_size;     /* Remaining size - in 4*32 bits */
static size_t play_sub_size;    /* size of current subtransfer */
static void dma_callback(void);
static int locked = 0;
static bool volatile is_playing = false;
static bool play_callback_pending = false;

#ifdef HAVE_RECORDING
/* Stopping playback gates clock if not recording */
static bool volatile is_recording = false;
#endif

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    ++locked;
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if(--locked == 0 && is_playing)
    {
        int old = disable_irq_save();
        if(play_callback_pending)
        {
            play_callback_pending = false;
            dma_callback();
        }
        restore_irq(old);
    }
}

static void play_start_pcm(void)
{
    const void *addr = dma_sub_addr;
    size_t size = dma_rem_size;
    if(size > MAX_TRANSFER)
        size = MAX_TRANSFER;

    play_sub_size = size;

    dma_enable_channel(0, (void*)addr, (void*)I2SOUT_DATA, DMA_PERI_I2SOUT,
                DMAC_FLOWCTRL_DMAC_MEM_TO_PERI, true, false, size >> 2,
                DMA_S1, dma_callback);
}

static void dma_callback(void)
{
    dma_sub_addr += play_sub_size;
    dma_rem_size -= play_sub_size;
    play_sub_size = 0; /* Might get called again if locked */

    if(locked)
    {
        play_callback_pending = is_playing;
        return;
    }

    if(!dma_rem_size)
    {
        if(!pcm_play_dma_complete_callback(PCM_DMAST_OK, &dma_start_addr,
                                           &dma_start_size))
            return;

        dma_sub_addr = dma_start_addr;
        dma_rem_size = dma_start_size;

        /* force writeback */
        commit_dcache_range(dma_start_addr, dma_start_size);
        play_start_pcm();
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    }
    else
    {
        play_start_pcm();
    }
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    is_playing = true;

    dma_start_addr = addr;
    dma_start_size = size;
    dma_sub_addr = dma_start_addr;
    dma_rem_size = size;

    dma_retain();

    /* force writeback */
    commit_dcache_range(dma_start_addr, dma_start_size);

    bitset32(&CGU_AUDIO, (1<<11));

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    is_playing = false;

    dma_disable_channel(0);

    /* Ensure byte counts read back 0 */
    DMAC_CH_SRC_ADDR(0) = 0;
    dma_start_addr = NULL;
    dma_start_size = 0;
    dma_rem_size = 0;

    dma_release();

#ifdef HAVE_RECORDING
    if (!is_recording)
        bitclr32(&CGU_AUDIO, (1<<11));
#endif

    play_callback_pending = false;
}

void pcm_play_dma_pause(bool pause)
{
    is_playing = !pause;

    if(pause)
    {
        dma_pause_channel(0);

        /* if producer's buffer finished, upper layer starts anew */
        if (dma_rem_size == 0)
            play_callback_pending = false;
    }
    else
    {
        if (play_sub_size != 0)
            dma_resume_channel(0);
        /* else unlock calls the callback if sub buffers remain */
    }
}

void pcm_play_dma_init(void)
{
    bitset32(&CGU_PERI, CGU_I2SOUT_APB_CLOCK_ENABLE);
    I2SOUT_CONTROL = (1<<6) | (1<<3);  /* enable dma, stereo */

    audiohw_preinit();
    pcm_dma_apply_settings();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
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

static inline unsigned char mclk_divider(void)
{
    return divider[pcm_fsel];
}

void pcm_dma_apply_settings(void)
{
    bitmod32(&CGU_AUDIO,
             (0<<24) |               /* I2SI_MCLK2PAD_EN = disabled */
             (0<<23) |               /* I2SI_MCLK_EN = disabled */
             (0<<14) |               /* I2SI_MCLK_DIV_SEL = unused */
             (0<<12) |               /* I2SI_MCLK_SEL = clk_main */
                                     /* I2SO_MCLK_EN = unchanged */
             (mclk_divider() << 2) | /* I2SO_MCLK_DIV_SEL */
             (AS3525_MCLK_SEL << 0), /* I2SO_MCLK_SEL */
             0x01fff7ff);
}

size_t pcm_get_bytes_waiting(void)
{
    int oldstatus = disable_irq_save();
    size_t addr = DMAC_CH_SRC_ADDR(0);
    size_t start_addr = (size_t)dma_start_addr;
    size_t start_size = dma_start_size;
    restore_interrupt(oldstatus);

    return start_size - addr + start_addr;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    int oldstatus = disable_irq_save();
    size_t addr = DMAC_CH_SRC_ADDR(0);
    size_t start_addr = (size_t)dma_start_addr;
    size_t start_size = dma_start_size;
    restore_interrupt(oldstatus);

    *count = (start_size - addr + start_addr) >> 2;
    return (void*)AS3525_UNCACHED_ADDR(addr);
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL)
        addr = AS3525_UNCACHED_ADDR(addr);
    return addr;
}
#endif


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING

static int rec_locked = 0;
static uint32_t *rec_dma_addr;
static size_t rec_dma_size;
static int keep_sample = 0; /* In nonzero, keep the sample; else, discard it */

void pcm_rec_lock(void)
{
    int oldlevel = disable_irq_save();

    if (++rec_locked == 1)
    {
        bitset32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE);
        VIC_INT_EN_CLEAR = INTERRUPT_I2SIN;
        I2SIN_MASK = 0; /* disables all interrupts */
    }

    restore_irq(oldlevel);
}


void pcm_rec_unlock(void)
{
    int oldlevel = disable_irq_save();

    if (--rec_locked == 0 && is_recording)
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
        while (rec_dma_size > 0)
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

                if (audio_output_source != AUDIO_SRC_PLAYBACK && !is_playing)
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
                rec_dma_size -= 4;
            }
        }
    }
    else
#endif /* CONFIG_CPU == AS3525 */
    {
        /* RX is stereo */
        while (rec_dma_size > 0)
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
                rec_dma_size -= 4;
            }
        }
    }

    /* Inform middle layer */
    if (pcm_rec_dma_complete_callback(PCM_DMAST_OK, (void **)&rec_dma_addr,
                                      &rec_dma_size))
    {
        pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
    }
}


void pcm_rec_dma_stop(void)
{
    is_recording = false;

    VIC_INT_EN_CLEAR = INTERRUPT_I2SIN;
    I2SIN_MASK = 0; /* disables all interrupts */

    rec_dma_addr = NULL;
    rec_dma_size = 0;

    if (!is_playing)
        bitclr32(&CGU_AUDIO, (1<<11));

    bitclr32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE);
}


void pcm_rec_dma_start(void *addr, size_t size)
{
    is_recording = true;

    bitset32(&CGU_AUDIO, (1<<11));

    rec_dma_addr = addr;
    rec_dma_size = size;

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


const void * pcm_rec_dma_get_peak_buffer(void)
{
    return rec_dma_addr;
}

#endif /* HAVE_RECORDING */
