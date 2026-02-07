/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#include "pcm.h"
#include "pcm_sampr.h"
#include "pcm-internal.h"
#include "audiohw.h"
#include "panic.h"
#include "nvic-arm.h"
#include "dmamux-stm32h743.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/sai.h"
#include "regs/stm32h743/dma.h"
#include "regs/stm32h743/dmamux.h"
#include <stdatomic.h>

/* Configurable bit depth */
#if PCM_NATIVE_BITDEPTH == 32
# define PCM_NATIVE_SAMPLE_SIZE     4
# define DMA_PSIZE_VAL              BV_DMA_CHN_CR_PSIZE_32BIT
# define SAI_DSIZE_VAL              BV_SAI_SUBBLOCK_CR1_DS_32BIT
# define SAI_FIFO_THRESH            BV_SAI_SUBBLOCK_CR2_FTH_QUARTER
# define SAI_FRL_VAL                64
#elif PCM_NATIVE_BITDEPTH == 24
# define PCM_NATIVE_SAMPLE_SIZE     4
# define DMA_PSIZE_VAL              BV_DMA_CHN_CR_PSIZE_32BIT
# define SAI_DSIZE_VAL              BV_SAI_SUBBLOCK_CR1_DS_24BIT
# define SAI_FIFO_THRESH            BV_SAI_SUBBLOCK_CR2_FTH_QUARTER
# define SAI_FRL_VAL                64
#elif PCM_NATIVE_BITDEPTH == 16
# define PCM_NATIVE_SAMPLE_SIZE     2
# define DMA_PSIZE_VAL              BV_DMA_CHN_CR_PSIZE_16BIT
# define SAI_DSIZE_VAL              BV_SAI_SUBBLOCK_CR1_DS_16BIT
# define SAI_FIFO_THRESH            BV_SAI_SUBBLOCK_CR2_FTH_HALF
# define SAI_FRL_VAL                32
#else
# error "Unsupported PCM bit depth"
#endif

static const uintptr_t sai1     = ITA_SAI1;
static const uintptr_t sai1a    = sai1 + ITO_SAI_A;
static const uintptr_t dmamux1  = ITA_DMAMUX1;
static const uintptr_t dma1     = ITA_DMA1;
static const uintptr_t dma1_ch0 = dma1 + ITO_DMA_CHN(0);

static atomic_size_t play_lock;
static volatile int play_active;

static int pcm_last_freq = -1;

static void play_dma_start(const void *addr, size_t size)
{
    commit_dcache_range(addr, size);

    /*
     * The number of transfers is defined by PSIZE, which may
     * be 16-bit or 32-bit depending on PCM native sample size.
     * Each FIFO write transfers 1 sample, so we want to set
     * NDT = size / PCM_NATIVE_SAMPLE_SIZE.
     *
     * When MSIZE == 32-bit and PSIZE == 16-bit, the DMA will
     * take each 32-bit word from memory and write the lower/upper
     * halfwords separately to the FIFO. The number of reads from
     * memory in this case is NDT/2, so we're still transferring
     * the correct amount of data.
     */
    reg_varl(dma1_ch0, DMA_CHN_NDTR) = size / PCM_NATIVE_SAMPLE_SIZE;
    reg_varl(dma1_ch0, DMA_CHN_M0AR) = (uintptr_t)addr;
    reg_writelf(dma1_ch0, DMA_CHN_CR, EN(1));

    pcm_play_dma_status_callback(PCM_DMAST_STARTED);
}

static void sai_init(void)
{
    /* Enable SAI1 and DMA1 */
    reg_writef(RCC_APB2ENR, SAI1EN(1));
    reg_writef(RCC_AHB1ENR, DMA1EN(1));

    /* Link DMA1_CH0 to SAI1A */
    reg_writelf(dmamux1, DMAMUX_CR(0), DMAREQ_ID(DMAMUX1_REQ_SAI1A_DMA));

    /* Configure DMA1 channel 0 */
    reg_writelf(dma1_ch0, DMA_CHN_CR,
                MBURST_V(INCR4),        /* 32-bit x 4 burst = 16 bytes */
                PBURST_V(INCR4),        /* (16|32)-bit x 4 burst = 8-16 bytes */
                TRBUFF(0),              /* bufferable mode not used for SAI */
                DBM(0),                 /* double buffer mode off */
                PL_V(VERYHIGH),         /* highest priority */
                MSIZE_V(32BIT),         /* read 32-bit words from memory */
                PSIZE(DMA_PSIZE_VAL),   /* set according to PCM bit depth */
                MINC(1),                /* increment memory address */
                PINC(0),                /* don't increment peripheral address */
                CIRC(0),                /* circular mode off */
                DIR_V(MEM_TO_PERIPH),   /* read from memory, write to SAI */
                PFCTRL_V(DMA),          /* DMA is flow controller */
                TCIE(1),                /* transfer complete interrupt */
                TEIE(1),                /* transfer error interrupt */
                DMEIE(1));              /* direct mode error interrupt */
    reg_writelf(dma1_ch0, DMA_CHN_FCR,
                FEIE(1),                /* fifo error interrupt */
                DMDIS(1),               /* enable fifo mode */
                FTH_V(FULL));           /* fifo threshold = 4 words */

    /* Set peripheral address here since it's constant */
    reg_varl(dma1_ch0, DMA_CHN_PAR) =
        (uintptr_t)reg_ptrl(sai1a, SAI_SUBBLOCK_DR);

    /* Configure SAI for playback */
    reg_writelf(sai1a, SAI_SUBBLOCK_CR1,
                OSR_V(256FS),                   /* MCLK is 256 x Fs */
                NOMCK(0),                       /* MCLK enabled */
                DMAEN(0),                       /* DMA request disabled */
                SAIEN(0),                       /* SAI disabled */
                OUTDRIV(0),                     /* drive outputs when SAIEN=1 */
                MONO(0),                        /* stereo mode */
                SYNCEN_V(ASYNC),                /* no sync with other SAIs */
                CKSTR_V(TX_FALLING_RX_RISING),  /* clock edge for sampling */
                LSBFIRST(0),                    /* transmit samples MSB first */
                DS(SAI_DSIZE_VAL),              /* sample size according to bit depth */
                PRTCFG_V(FREE),                 /* free protocol */
                MODE_V(MASTER_TX));             /* operate as bus master */
    reg_writelf(sai1a, SAI_SUBBLOCK_CR2,
                MUTEVAL_V(ZERO_SAMPLE),         /* send zero sample on mute */
                MUTE(1),                        /* mute output initially */
                TRIS(0),                        /* don't tri-state outputs */
                FTH(SAI_FIFO_THRESH));          /* fifo threshold (2 or 4 samples) */
    reg_writelf(sai1a, SAI_SUBBLOCK_FRCR,
                FSOFF(1), FSPOL(0), FSDEF(1),   /* I2S format */
                FSALL(SAI_FRL_VAL/2 - 1),       /* FS active for half the frame */
                FRL(SAI_FRL_VAL - 1));          /* set frame length */
    reg_writelf(sai1a, SAI_SUBBLOCK_SLOTR,
                SLOTEN(3), NBSLOT(2 - 1),       /* enable first two slots */
                SLOTSZ_V(DATASZ),               /* slot size = data size */
                FBOFF(0));                      /* no bit offset in slot */

    /* Enable interrupts in NVIC */
    nvic_enable_irq(NVIC_IRQN_DMA1_STR0);
}

struct div_settings
{
    uint8_t pllm;
    uint8_t plln;
    uint8_t pllp;
    uint8_t mckdiv;
};

_Static_assert(STM32_HSE_FREQ == 24000000,
               "Audio clock settings only valid for 24MHz HSE");

static const struct div_settings div_settings[] = {
    [HW_FREQ_96] = { .pllm = 5,  .plln = 128 - 1, .pllp = 25 - 1,  .mckdiv = 0 }, /* exact */
    [HW_FREQ_48] = { .pllm = 5,  .plln = 128 - 1, .pllp = 25 - 1,  .mckdiv = 2 }, /* exact */
    [HW_FREQ_24] = { .pllm = 5,  .plln = 128 - 1, .pllp = 25 - 1,  .mckdiv = 4 }, /* exact */
    [HW_FREQ_12] = { .pllm = 5,  .plln = 128 - 1, .pllp = 25 - 1,  .mckdiv = 8 }, /* exact */

    [HW_FREQ_88] = { .pllm = 4,  .plln = 143 - 1, .pllp = 38 - 1,  .mckdiv = 0 }, /* -11ppm error */
    [HW_FREQ_44] = { .pllm = 4,  .plln = 143 - 1, .pllp = 38 - 1,  .mckdiv = 2 }, /* -11ppm error */
    [HW_FREQ_22] = { .pllm = 5,  .plln = 147 - 1, .pllp = 125 - 1, .mckdiv = 0 }, /* exact */
    [HW_FREQ_11] = { .pllm = 5,  .plln = 147 - 1, .pllp = 125 - 1, .mckdiv = 2 }, /* exact */

    [HW_FREQ_64] = { .pllm = 15, .plln = 256 - 1, .pllp = 25 - 1,  .mckdiv = 0 }, /* exact */
    [HW_FREQ_32] = { .pllm = 15, .plln = 256 - 1, .pllp = 25 - 1,  .mckdiv = 2 }, /* exact */
    [HW_FREQ_16] = { .pllm = 15, .plln = 256 - 1, .pllp = 25 - 1,  .mckdiv = 4 }, /* exact */
    [HW_FREQ_8 ] = { .pllm = 15, .plln = 256 - 1, .pllp = 25 - 1,  .mckdiv = 8 }, /* exact */
};

static void sai_set_pll(const struct div_settings *div)
{
    /* Disable the PLL so it can be reconfigured */
    if (reg_readf(RCC_CR, PLL2RDY))
    {
        reg_writef(RCC_CR, PLL2ON(0));
        while (reg_readf(RCC_CR, PLL2RDY));
        while (reg_readf(RCC_CR, PLL2ON));
    }

    /* Set the PLL configuration */
    uint32_t pllcfgr = reg_var(RCC_PLLCFGR);

    reg_writef(RCC_PLLCKSELR,
               DIVM2(div->pllm));
    reg_writef(RCC_PLL2DIVR,
               DIVN(div->plln),
               DIVP(div->pllp),
               DIVQ(0), DIVR(0));
    reg_vwritef(pllcfgr, RCC_PLLCFGR,
                DIVP2EN(1),
                DIVQ2EN(0),
                DIVR2EN(0),
                PLL2FRACEN(0),
                PLL2VCOSEL_V(WIDE));

    if (div->pllm <= 6)
        reg_vwritef(pllcfgr, RCC_PLLCFGR, PLL2RGE_V(4_8MHZ));
    else if (div->pllm <= 12)
        reg_vwritef(pllcfgr, RCC_PLLCFGR, PLL2RGE_V(2_4MHZ));
    else
        reg_vwritef(pllcfgr, RCC_PLLCFGR, PLL2RGE_V(1_2MHZ));

    reg_var(RCC_PLLCFGR) = pllcfgr;

    /* Enable the PLL again */
    reg_writef(RCC_CR, PLL2ON(1));
    while (!reg_readf(RCC_CR, PLL2RDY));
}

static void sai_set_frequency(int freq)
{
    /* Can't change frequency while the SAI is active */
    if (reg_readlf(sai1a, SAI_SUBBLOCK_CR1, SAIEN))
        panicf("%s while SAI active", __func__);

    const struct div_settings *div = &div_settings[freq];
    const struct div_settings *old_div = NULL;

    if (pcm_last_freq >= 0)
        old_div = &div_settings[pcm_last_freq];

    if (old_div == NULL ||
        div->pllm != old_div->pllm ||
        div->plln != old_div->plln ||
        div->pllp != old_div->pllp)
    {
        sai_set_pll(div);
    }

    /* Configure the MCK divider in SAI */
    reg_writelf(sai1a, SAI_SUBBLOCK_CR1, MCKDIV(div->mckdiv));
}

static void sink_dma_init(void)
{
    sai_init();
    audiohw_init();
}

static void sink_set_freq(uint16_t freq)
{
    /*
     * Muting here doesn't seem to help clicks when switching
     * tracks of different frequencies; the audio may need to
     * be soft-muted in software before the switch.
     */
    audiohw_mute(true);

    sai_set_frequency(freq);
    pcm_last_freq = freq;

    audiohw_mute(false);
}

static void sink_dma_start(const void *addr, size_t size)
{
    play_active = true;

    play_dma_start(addr, size);
    reg_writelf(sai1a, SAI_SUBBLOCK_CR1, SAIEN(1), DMAEN(1));
    reg_writelf(sai1a, SAI_SUBBLOCK_CR2, MUTE(0));
}

static void sink_dma_stop(void)
{
    play_active = false;

    reg_writelf(sai1a, SAI_SUBBLOCK_CR2, MUTE(1));
    reg_writelf(sai1a, SAI_SUBBLOCK_CR1, SAIEN(0), DMAEN(0));
    reg_writelf(dma1_ch0, DMA_CHN_CR, EN(0));

    /* Must wait for SAIEN bit to be cleared */
    while (reg_readlf(sai1a, SAI_SUBBLOCK_CR1, SAIEN));
}

static void sink_lock(void)
{
    /* Disable IRQ on first lock */
    if (atomic_fetch_add_explicit(&play_lock, 1, memory_order_relaxed) == 0)
        nvic_disable_irq_sync(NVIC_IRQN_DMA1_STR0);
}

static void sink_unlock(void)
{
    /* Enable IRQ on release of last lock */
    if (atomic_fetch_sub_explicit(&play_lock, 1, memory_order_relaxed) == 1)
        nvic_enable_irq(NVIC_IRQN_DMA1_STR0);
}

struct pcm_sink builtin_pcm_sink = {
    .caps = {
        .samprs       = hw_freq_sampr,
        .num_samprs   = HW_NUM_FREQ,
        .default_freq = HW_FREQ_DEFAULT,
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

void dma1_ch0_irq_handler(void)
{
    uint32_t lisr = reg_varl(dma1, DMA_LISR);
    const void *addr;
    size_t size;

    if (reg_vreadf(lisr, DMA_LISR, TEIF0) ||
        reg_vreadf(lisr, DMA_LISR, DMEIF0) ||
        reg_vreadf(lisr, DMA_LISR, FEIF0))
    {
        reg_assignlf(dma1, DMA_LIFCR, TEIF0(1), DMEIF0(1), FEIF0(1));
        reg_writelf(dma1_ch0, DMA_CHN_CR, EN(0));

        pcm_play_dma_status_callback(PCM_DMAST_ERR_DMA);
    }
    else if (reg_vreadf(lisr, DMA_LISR, TCIF0))
    {
        reg_assignlf(dma1, DMA_LIFCR, TCIF0(1));

        /* If we call the complete callback while not playing
         * it can cause the PCM layer to get stuck... somehow */
        if (!play_active)
            return;

        if (pcm_play_dma_complete_callback(PCM_DMAST_OK, &addr, &size))
            play_dma_start(addr, size);
    }
    else
    {
        panicf("%s: %08lx", __func__, lisr);
    }
}
