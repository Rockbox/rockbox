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

#define MAX_TRANSFER (4*((1<<11)-1)) /* maximum data we can transfer via DMA
                                      * i.e. 32 bits at once (size of I2SO_DATA)
                                      * and the number of 32bits words has to
                                      * fit in 11 bits of DMA register */

static void *dma_start_addr;    /* Pointer to callback buffer */
static size_t dma_start_size;   /* Size of callback buffer */
static void *dma_sub_addr;      /* Pointer to sub buffer */
static size_t dma_rem_size;     /* Remaining size - in 4*32 bits */
static size_t play_sub_size;    /* size of current subtransfer */
static void dma_callback(void);
static int locked = 0;
static bool is_playing = false;
static bool play_callback_pending = false;

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

    dma_enable_channel(1, (void*)addr, (void*)I2SOUT_DATA, DMA_PERI_I2SOUT,
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
        pcm_play_get_more_callback(&dma_start_addr, &dma_start_size);

        if (!dma_start_size)
            return;

        dma_sub_addr = dma_start_addr;
        dma_rem_size = dma_start_size;

        /* force writeback */
        clean_dcache_range(dma_start_addr, dma_start_size);
    }

    play_start_pcm();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_start_addr = (void*)addr;
    dma_start_size = size;
    dma_sub_addr = dma_start_addr;
    dma_rem_size = size;

    bitset32(&CGU_PERI, CGU_I2SOUT_APB_CLOCK_ENABLE);
    CGU_AUDIO |= (1<<11);

    dma_retain();

    is_playing = true;

    /* force writeback */
    clean_dcache_range(dma_start_addr, dma_start_size);
    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    is_playing = false;
    dma_disable_channel(1);

    /* Ensure byte counts read back 0 */
    DMAC_CH_SRC_ADDR(1) = 0;
    dma_start_addr = NULL;
    dma_start_size = 0;
    dma_rem_size = 0;

    dma_release();

    bitclr32(&CGU_PERI, CGU_I2SOUT_APB_CLOCK_ENABLE);
    CGU_AUDIO &= ~(1<<11);

    play_callback_pending = false;
}

void pcm_play_dma_pause(bool pause)
{
    is_playing = !pause;

    if(pause)
    {
        dma_pause_channel(1);

        /* if producer's buffer finished, upper layer starts anew */
        if (dma_rem_size == 0)
            play_callback_pending = false;
    }
    else
    {
        if (play_sub_size != 0)
            dma_resume_channel(1);
        /* else unlock calls the callback if sub buffers remain */
    }
}

void pcm_play_dma_init(void)
{
    bitset32(&CGU_PERI, CGU_I2SOUT_APB_CLOCK_ENABLE);

    I2SOUT_CONTROL = (1<<6)|(1<<3)  /* enable dma, stereo */;

    audiohw_preinit();
}

void pcm_postinit(void)
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
    int cgu_audio = CGU_AUDIO;              /* read register */
    cgu_audio &= ~(3 << 0);                 /* clear i2sout MCLK_SEL */
    cgu_audio |=  (AS3525_MCLK_SEL << 0);   /* set i2sout MCLK_SEL */
    cgu_audio &= ~(0x1ff << 2);             /* clear i2sout divider */
    cgu_audio |= mclk_divider() << 2;       /* set new i2sout divider */
    cgu_audio &= ~(1 << 23);                /* clear I2SI_MCLK_EN */
    cgu_audio &= ~(1 << 24);                /* clear I2SI_MCLK2PAD_EN */
    CGU_AUDIO = cgu_audio;                  /* write back register */
}

size_t pcm_get_bytes_waiting(void)
{
    int oldstatus = disable_irq_save();
    size_t addr = DMAC_CH_SRC_ADDR(1);
    size_t start_addr = (size_t)dma_start_addr;
    size_t start_size = dma_start_size;
    restore_interrupt(oldstatus);

    return start_size - addr + start_addr;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    int oldstatus = disable_irq_save();
    size_t addr = DMAC_CH_SRC_ADDR(1);
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
static bool is_recording = false;
static bool rec_callback_pending = false;
static void *rec_dma_start_addr;
static size_t rec_dma_size, rec_dma_transfer_size;
static void rec_dma_callback(void);
#if CONFIG_CPU == AS3525
/* points to the samples which need to be duplicated into the right channel */
static int16_t *mono_samples;
#endif


void pcm_rec_lock(void)
{
    ++rec_locked;
}


void pcm_rec_unlock(void)
{
    if(--rec_locked == 0 && is_recording)
    {
        int old = disable_irq_save();
        if(rec_callback_pending)
        {
            rec_callback_pending = false;
            rec_dma_callback();
        }
        restore_irq(old);
    }
}


static void rec_dma_start(void)
{
    rec_dma_transfer_size = rec_dma_size;

    /* We are limited to 8188 DMA transfers, and the recording core asks for
     * 8192 bytes. Avoid splitting 8192 bytes transfers in 8188 + 4 */
    if(rec_dma_transfer_size > 4096)
        rec_dma_transfer_size = 4096;

    dma_enable_channel(1, (void*)I2SIN_DATA, rec_dma_start_addr, DMA_PERI_I2SIN,
                DMAC_FLOWCTRL_DMAC_PERI_TO_MEM, false, true,
                rec_dma_transfer_size >> 2, DMA_S4, rec_dma_callback);
}


#if CONFIG_CPU == AS3525
/* if needed, duplicate samples of the working channel until the given bound */
static inline void mono2stereo(int16_t *end)
{
    if(audio_channels != 1) /* only for microphone */
        return;
#if 0
    /* load pointer in a register and avoid updating it in each loop */
    register int16_t *samples = mono_samples;

    do {
        int16_t left = *samples++;  // load 1 sample of the left-channel
        *samples++ = left;          // copy it in the right-channel
    } while(samples != end);

    mono_samples = samples; /* update pointer */
#else
    /* gcc doesn't use pre indexing : let's save 1 cycle */
    int16_t left;
    asm (
        "1: ldrh %0, [%1], #2   \n" // load 1 sample of the left-channel
        "   strh %0, [%1], #2   \n" // copy it in the right-channel
        "   cmp %1, %2          \n" // are we finished?
        "   bne  1b             \n"
        : "=&r"(left), "+r"(mono_samples)
        : "r"(end)
        : "memory"
    );
#endif /* C / ASM */
}
#endif /* CONFIG_CPU == AS3525 */

static void rec_dma_callback(void)
{
    if(rec_dma_transfer_size)
    {
        rec_dma_size -= rec_dma_transfer_size;
        rec_dma_start_addr += rec_dma_transfer_size;

        /* don't act like we just transferred data when we are called from
         * pcm_rec_unlock() */
        rec_dma_transfer_size = 0;

#if CONFIG_CPU == AS3525
        /* the 2nd channel is silent when recording microphone on as3525v1 */
        mono2stereo(AS3525_UNCACHED_ADDR((int16_t*)rec_dma_start_addr));
#endif

        if(locked)
        {
            rec_callback_pending = is_recording;
            return;
        }
    }

    if(!rec_dma_size)
    {
        pcm_rec_more_ready_callback(0, &rec_dma_start_addr,
                                    &rec_dma_size);

        if(rec_dma_size == 0)
            return;

        dump_dcache_range(rec_dma_start_addr, rec_dma_size);
#if CONFIG_CPU == AS3525
        mono_samples = AS3525_UNCACHED_ADDR((int16_t*)rec_dma_start_addr);
#endif
    }

    rec_dma_start();
}

void pcm_rec_dma_stop(void)
{
    is_recording = false;
    dma_disable_channel(1);
    dma_release();
    rec_dma_size = 0;

    I2SIN_CONTROL &= ~(1<<11); /* disable dma */

    CGU_AUDIO &= ~(1<<11);
    bitclr32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE |
                        CGU_I2SOUT_APB_CLOCK_ENABLE);

    rec_callback_pending = false;
}


void pcm_rec_dma_start(void *addr, size_t size)
{
    dump_dcache_range(addr, size);
    rec_dma_start_addr = addr;
#if CONFIG_CPU == AS3525
    mono_samples = AS3525_UNCACHED_ADDR(addr);
#endif
    rec_dma_size = size;

    dma_retain();

    bitset32(&CGU_PERI, CGU_I2SIN_APB_CLOCK_ENABLE |
                        CGU_I2SOUT_APB_CLOCK_ENABLE);
    CGU_AUDIO |= (1<<11);

    I2SIN_CONTROL |= (1<<11)|(1<<5); /* enable dma, 14bits samples */

    is_recording = true;

    rec_dma_start();
}


void pcm_rec_dma_close(void)
{
}


void pcm_rec_dma_init(void)
{
    /* i2c clk src = I2SOUTIF, sdata src = AFE,
     * data valid at positive edge of SCLK */
    I2SIN_CONTROL = (1<<2);
    I2SIN_MASK = 0; /* disables all interrupts */
}


const void * pcm_rec_dma_get_peak_buffer(void)
{
#if CONFIG_CPU == AS3525
    /*
     * We need to prevent the DMA callback from kicking in while we are
     * faking the right channel with data from left channel.
     */

    int old = disable_irq_save();
    int16_t *addr = AS3525_UNCACHED_ADDR((int16_t *)DMAC_CH_DST_ADDR(1));
    mono2stereo(addr);
    restore_irq(old);

    return addr;

#else
    /* Microphone recording is stereo on as3525v2 */
    return AS3525_UNCACHED_ADDR((int16_t *)DMAC_CH_DST_ADDR(1));
#endif
}

#endif /* HAVE_RECORDING */
