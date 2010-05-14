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

static unsigned char *dma_start_addr;
static size_t      dma_size;   /* in 4*32 bits */
static void dma_callback(void);
static int locked = 0;

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if(++locked == 1)
        VIC_INT_EN_CLEAR = INTERRUPT_DMAC;
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if(--locked == 0)
        VIC_INT_ENABLE = INTERRUPT_DMAC;
}

static void play_start_pcm(void)
{
    const unsigned char* addr = dma_start_addr;
    size_t size = dma_size;
    if(size > MAX_TRANSFER)
        size = MAX_TRANSFER;

    dma_size -= size;
    dma_start_addr += size;

    clean_dcache_range((void*)addr, size);  /* force write back */
    dma_enable_channel(1, (void*)addr, (void*)I2SOUT_DATA, DMA_PERI_I2SOUT,
                DMAC_FLOWCTRL_DMAC_MEM_TO_PERI, true, false, size >> 2, DMA_S1,
                dma_callback);
}

static void dma_callback(void)
{
    if(!dma_size)
    {
        register pcm_more_callback_type get_more = pcm_callback_for_more;
        if(get_more)
            get_more(&dma_start_addr, &dma_size);
    }

    if(!dma_size)
    {
        pcm_play_dma_stop();
        pcm_play_dma_stopped_callback();
    }
    else
        play_start_pcm();
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_size = size;
    dma_start_addr = (unsigned char*)addr;

    CGU_PERI |= CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO |= (1<<11);

    dma_retain();

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    dma_disable_channel(1);
    dma_size = 0;

    dma_release();

    CGU_PERI &= ~CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO &= ~(1<<11);
}

void pcm_play_dma_pause(bool pause)
{
    if(pause)
        dma_disable_channel(1);
    else
        play_start_pcm();
}

void pcm_play_dma_init(void)
{
    CGU_PERI |= CGU_I2SOUT_APB_CLOCK_ENABLE;

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
    CGU_AUDIO = cgu_audio;                  /* write back register */
}

size_t pcm_get_bytes_waiting(void)
{
    return dma_size;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    *count = dma_size >> 2;
    return (const void*)dma_start_addr;
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL)
        addr = UNCACHED_ADDR(addr);
    return addr;
}
#endif


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING

static int rec_locked = 0;
static unsigned char *rec_dma_start_addr;
static size_t rec_dma_size, rec_dma_transfer_size;
static void rec_dma_callback(void);
#if CONFIG_CPU == AS3525
/* points to the samples which need to be duplicated into the right channel */
static int16_t *mono_samples;
#endif


void pcm_rec_lock(void)
{
    if(++rec_locked == 1)
        VIC_INT_EN_CLEAR = INTERRUPT_DMAC;
}


void pcm_rec_unlock(void)
{
    if(--rec_locked == 0)
        VIC_INT_ENABLE = INTERRUPT_DMAC;
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


/* if needed, duplicate samples of the working channel until the given bound */
static inline void mono2stereo(int16_t *end)
{
#if CONFIG_CPU == AS3525
    if(audio_channels != 1) /* only for microphone */
        return;
#if 0
    do {
        int16_t left = *mono_samples++;
        *mono_samples++ = left;
    } while(mono_samples != end);
#else
    /* gcc doesn't use pre indexing and load/store mono_samples at each loop
     * let's save some cycles with a smaller loop */
    int16_t tmp;
    asm (
        "1: ldrh %0, [%1], #2   \n"
        "   strh %0, [%1], #2   \n"
        "   cmp %1, %2          \n"
        "   bne  1b             \n"
    : "=r"(tmp), "+r"(mono_samples)
    : "r"(end)
    : "memory"
    );
#endif /* C / ASM */
#else
    /* microphone recording is stereo on as3525v2 */
    (void)end;
#endif
}

static void rec_dma_callback(void)
{
    rec_dma_size -= rec_dma_transfer_size;
    rec_dma_start_addr += rec_dma_transfer_size;

    /* the 2nd channel is silent when recording microphone on as3525v1 */
    mono2stereo(UNCACHED_ADDR((int16_t*)rec_dma_start_addr));

    if(!rec_dma_size)
    {
        register pcm_more_callback_type2 more_ready = pcm_callback_more_ready;
        if (!more_ready || more_ready(0) < 0)
        {
            /* Finished recording */
            pcm_rec_dma_stop();
            pcm_rec_dma_stopped_callback();
            return;
        }
    }

    rec_dma_start();
}


void pcm_rec_dma_record_more(void *start, size_t size)
{
    dump_dcache_range(start, size);
    rec_dma_start_addr = start;
#if CONFIG_CPU == AS3525
    mono_samples = UNCACHED_ADDR(start);
#endif
    rec_dma_size = size;
}


void pcm_rec_dma_stop(void)
{
    dma_disable_channel(1);
    dma_release();
    rec_dma_size = 0;

    I2SOUT_CONTROL &= ~(1<<5); /* source = i2soutif fifo */
    I2SIN_CONTROL &= ~(1<<11); /* disable dma */

    CGU_AUDIO &= ~((1<<23)|(1<<11));
    CGU_PERI &= ~(CGU_I2SIN_APB_CLOCK_ENABLE|CGU_I2SOUT_APB_CLOCK_ENABLE);
}


void pcm_rec_dma_start(void *addr, size_t size)
{
    dump_dcache_range(addr, size);
    rec_dma_start_addr = addr;
#if CONFIG_CPU == AS3525
    mono_samples = UNCACHED_ADDR(addr);
#endif
    rec_dma_size = size;

    dma_retain();

    CGU_PERI |= CGU_I2SIN_APB_CLOCK_ENABLE|CGU_I2SOUT_APB_CLOCK_ENABLE;
    CGU_AUDIO |= ((1<<23)|(1<<11));

    I2SOUT_CONTROL |= 1<<5; /* source = loopback from i2sin fifo */

    I2SIN_CONTROL |= (1<<11)|(1<<5); /* enable dma, 14bits samples */

    rec_dma_start();
}


void pcm_rec_dma_close(void)
{
}


void pcm_rec_dma_init(void)
{
    int cgu_audio = CGU_AUDIO;              /* read register */
    cgu_audio &= ~(3 << 12);                /* clear i2sin MCLK_SEL */
    cgu_audio |=  (AS3525_MCLK_SEL << 12);  /* set i2sin MCLK_SEL */
    cgu_audio &= ~(0x1ff << 14);            /* clear i2sin divider */
    cgu_audio |= mclk_divider() << 14;      /* set new i2sin divider */
    CGU_AUDIO = cgu_audio;                  /* write back register */

    /* i2c clk src = I2SOUTIF, sdata src = AFE,
     * data valid at positive edge of SCLK */
    I2SIN_CONTROL = (1<<2);
}


const void * pcm_rec_dma_get_peak_buffer(void)
{
    pcm_rec_lock();
    int16_t *addr = UNCACHED_ADDR((int16_t *)DMAC_CH_DST_ADDR(1));
    mono2stereo(addr);
    pcm_rec_unlock();

    return addr;
}

#endif /* HAVE_RECORDING */
