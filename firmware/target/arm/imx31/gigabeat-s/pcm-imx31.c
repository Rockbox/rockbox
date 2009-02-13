/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "audio.h"
#include "sound.h"
#include "clkctl-imx31.h"
#include "sdma-imx31.h"
#include "mmu-imx31.h"

#define DMA_PLAY_CH_NUM 2
#define DMA_REC_CH_NUM 1

static struct buffer_descriptor dma_play_bd DEVBSS_ATTR;
static struct channel_descriptor dma_play_cd DEVBSS_ATTR;

struct dma_data
{
    int locked;
    int callback_pending; /* DMA interrupt happened while locked */
    int state;
};

static struct dma_data dma_play_data =
{
    /* Initialize to a locked, stopped state */
    .locked = 0,
    .callback_pending = 0,
    .state = 0
};

static void play_dma_callback(void)
{
    unsigned char *start;
    size_t size = 0;
    pcm_more_callback_type get_more = pcm_callback_for_more;

    if (dma_play_data.locked != 0)
    {
        /* Callback is locked out */
        dma_play_data.callback_pending = dma_play_data.state;
        return;
    }

    if (dma_play_bd.mode.status & BD_RROR)
    {
        /* Stop on error */
    }
    else if (get_more != NULL && (get_more(&start, &size), size != 0))
    {
        start = (void*)(((unsigned long)start + 3) & ~3);
        size &= ~3;

        /* Flush any pending cache writes */
        clean_dcache_range(start, size);
        dma_play_bd.buf_addr = (void *)addr_virt_to_phys((unsigned long)start);
        dma_play_bd.mode.count = size;
        dma_play_bd.mode.command = TRANSFER_16BIT;
        dma_play_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;
        sdma_channel_run(DMA_PLAY_CH_NUM);
        return;
    }

    /* Error, callback missing or no more DMA to do */
    pcm_play_dma_stop();
    pcm_play_dma_stopped_callback();
}

void pcm_play_lock(void)
{
    if (++dma_play_data.locked == 1)
        imx31_regclr32(&SSI_SIER1, SSI_SIER_TDMAE);
}

void pcm_play_unlock(void)
{
    if (--dma_play_data.locked == 0 && dma_play_data.state != 0)
    {
        int oldstatus = disable_irq_save();
        int pending = dma_play_data.callback_pending;
        dma_play_data.callback_pending = 0;
        SSI_SIER1 |= SSI_SIER_TDMAE;
        restore_irq(oldstatus);

        /* Should an interrupt be forced instead? The upper pcm layer can
         * call producer's callback in thread context so technically this is
         * acceptable. */
        if (pending != 0)
            play_dma_callback();
    }
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

void pcm_play_dma_init(void)
{
    /* Init channel information */
    dma_play_cd.bd_count = 1;
    dma_play_cd.callback = play_dma_callback;
    dma_play_cd.shp_addr = SDMA_PER_ADDR_SSI1_TX1;
    dma_play_cd.wml      = SDMA_SSI_TXFIFO_WML*2;
    dma_play_cd.per_type = SDMA_PER_SSI;
    dma_play_cd.tran_type = SDMA_TRAN_EMI_2_PER;
    dma_play_cd.event_id1 = SDMA_REQ_SSI1_TX1;

    sdma_channel_init(DMA_PLAY_CH_NUM, &dma_play_cd, &dma_play_bd);

    imx31_clkctl_module_clock_gating(CG_SSI1, CGM_ON_ALL);
    imx31_clkctl_module_clock_gating(CG_SSI2, CGM_ON_ALL);

    /* Reset & disable SSIs */
    SSI_SCR2 &= ~SSI_SCR_SSIEN;
    SSI_SCR1 &= ~SSI_SCR_SSIEN;

    SSI_SIER1 = 0;
    SSI_SIER2 = 0;

    /* Set up audio mux */

    /* Port 1 (internally connected to SSI1)
     * All clocking is output sourced from port 4 */
    AUDMUX_PTCR1 = AUDMUX_PTCR_TFS_DIR | AUDMUX_PTCR_TFSEL_PORT4 |
                   AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT4 |
                   AUDMUX_PTCR_SYN;
 
    /* Receive data from port 4 */
    AUDMUX_PDCR1 = AUDMUX_PDCR_RXDSEL_PORT4;
    /* All clock lines are inputs sourced from the master mode codec and
     * sent back to SSI1 through port 1 */
    AUDMUX_PTCR4 = AUDMUX_PTCR_SYN;

    /* Receive data from port 1 */
    AUDMUX_PDCR4 = AUDMUX_PDCR_RXDSEL_PORT1;

    /* PORT2 (internally connected to SSI2) routes clocking to PORT5 to
     * provide MCLK to the codec */
    /* TX clocks are inputs taken from SSI2 */
    /* RX clocks are outputs taken from PORT4 */
    AUDMUX_PTCR2 = AUDMUX_PTCR_RFS_DIR | AUDMUX_PTCR_RFSSEL_PORT4 |
                   AUDMUX_PTCR_RCLKDIR | AUDMUX_PTCR_RCSEL_PORT4;
    /* RX data taken from PORT4 */
    AUDMUX_PDCR2 = AUDMUX_PDCR_RXDSEL_PORT4;

    /* PORT5 outputs TCLK sourced from PORT2 (SSI2) */
    AUDMUX_PTCR5 = AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT2;
    AUDMUX_PDCR5 = 0;

    /* Setup SSIs */

    /* SSI1 - SoC software interface for all I2S data out */
    SSI_SCR1 = SSI_SCR_SYN | SSI_SCR_I2S_MODE_SLAVE;
    SSI_STCR1 = SSI_STCR_TXBIT0 | SSI_STCR_TSCKP | SSI_STCR_TFSI |
                SSI_STCR_TEFS | SSI_STCR_TFEN0;

    /* 16 bits per word, 2 words per frame */
    SSI_STCCR1 = SSI_STRCCR_WL16 | SSI_STRCCR_DCw(2-1) |
                 SSI_STRCCR_PMw(4-1);

    /* Transmit low watermark */
    SSI_SFCSR1 = (SSI_SFCSR1 & ~SSI_SFCSR_TFWM0) |
                 SSI_SFCSR_TFWM0w(8-SDMA_SSI_TXFIFO_WML);
    SSI_STMSK1 = 0;

    /* SSI2 - provides MCLK to codec. Receives data from codec. */
    SSI_STCR2 = SSI_STCR_TXDIR;

    /* f(INT_BIT_CLK) =
     * f(SYS_CLK) / [(DIV2 + 1)*(7*PSR + 1)*(PM + 1)*2] =
     * 677737600 / [(1 + 1)*(7*0 + 1)*(0 + 1)*2] =
     * 677737600 / 4 = 169344000 Hz
     *
     * 45.4.2.2 DIV2, PSR, and PM Bit Description states:
     * Bits DIV2, PSR, and PM should not be all set to zero at the same
     * time.
     *
     * The hardware seems to force a divide by 4 even if all bits are
     * zero but comply by setting DIV2 and the others to zero.
     */
    SSI_STCCR2 = SSI_STRCCR_DIV2 | SSI_STRCCR_PMw(1-1);

    /* SSI2 - receive - asynchronous clocks */
    SSI_SCR2 = SSI_SCR_I2S_MODE_SLAVE;

    SSI_SRCR2 = SSI_SRCR_RXBIT0 | SSI_SRCR_RSCKP | SSI_SRCR_RFSI |
                SSI_SRCR_REFS;

    /* 16 bits per word, 2 words per frame */
    SSI_SRCCR2 = SSI_STRCCR_WL16 | SSI_STRCCR_DCw(2-1) |
                 SSI_STRCCR_PMw(4-1);

    /* Receive high watermark */
    SSI_SFCSR2 = (SSI_SFCSR2 & ~SSI_SFCSR_RFWM0) |
                 SSI_SFCSR_RFWM0w(SDMA_SSI_RXFIFO_WML);
    SSI_SRMSK2 = 0;

    /* Enable SSI2 (codec clock) */
    SSI_SCR2 |= SSI_SCR_SSIEN;

    audiohw_init();
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

static void play_start_pcm(void)
{
    /* Stop transmission (if in progress) */
    SSI_SCR1 &= ~SSI_SCR_TE;

    SSI_SCR1 |= SSI_SCR_SSIEN;   /* Enable SSI */
    SSI_STCR1 |= SSI_STCR_TFEN0; /* Enable TX FIFO */

    dma_play_data.state = 1; /* Enable DMA requests on unlock */

    /* Do prefill to prevent swapped channels (see TLSbo61214 in MCIMX31CE).
     * No actual solution was offered but this appears to work. */
    SSI_STX0_1 = 0;
    SSI_STX0_1 = 0;
    SSI_STX0_1 = 0;
    SSI_STX0_1 = 0;

    SSI_SCR1 |= SSI_SCR_TE;  /* Start transmitting */
}

static void play_stop_pcm(void)
{
    /* Wait for FIFO to empty */
    while (SSI_SFCSR_TFCNT0r(SSI_SFCSR1) > 0);

    /* Disable transmission */
    SSI_STCR1 &= ~SSI_STCR_TFEN0;
    SSI_SCR1 &= ~(SSI_SCR_TE | SSI_SCR_SSIEN);

    /* Set state before pending to prevent race with interrupt */
    /* Do not enable DMA requests on unlock */
    dma_play_data.state = 0;
    dma_play_data.callback_pending = 0;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    sdma_channel_stop(DMA_PLAY_CH_NUM);

    /* Disable transmission */
    SSI_STCR1 &= ~SSI_STCR_TFEN0;
    SSI_SCR1 &= ~(SSI_SCR_TE | SSI_SCR_SSIEN);

    addr = (void *)(((unsigned long)addr + 3) & ~3);
    size &= ~3;

    if (size <= 0)
        return;

    if (!sdma_channel_reset(DMA_PLAY_CH_NUM))
        return;

    clean_dcache_range(addr, size);
    dma_play_bd.buf_addr =
        (void *)addr_virt_to_phys((unsigned long)(void *)addr);
    dma_play_bd.mode.count = size;
    dma_play_bd.mode.command = TRANSFER_16BIT;
    dma_play_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;

    play_start_pcm();
    sdma_channel_run(DMA_PLAY_CH_NUM);
}

void pcm_play_dma_stop(void)
{
    sdma_channel_stop(DMA_PLAY_CH_NUM);
    play_stop_pcm();
}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        sdma_channel_pause(DMA_PLAY_CH_NUM);
        play_stop_pcm();
    }
    else
    {
        play_start_pcm();
        sdma_channel_run(DMA_PLAY_CH_NUM);
    }
}

/* Return the number of bytes waiting - full L-R sample pairs only */
size_t pcm_get_bytes_waiting(void)
{
    static unsigned long dsa DEVBSS_ATTR;
    long offs, size;
    int oldstatus;

    /* read burst dma source address register in channel context */
    sdma_read_words(&dsa, CHANNEL_CONTEXT_ADDR(DMA_PLAY_CH_NUM)+0x0b, 1);

    oldstatus = disable_irq_save();
    offs = dsa - (unsigned long)dma_play_bd.buf_addr;
    size = dma_play_bd.mode.count;
    restore_irq(oldstatus);

    /* Be addresses are coherent (no buffer change during read) */
    if (offs >= 0 && offs < size)
    {
        return (size - offs) & ~3;
    }

    return 0;
}

/* Return a pointer to the samples and the number of them in *count */
const void * pcm_play_dma_get_peak_buffer(int *count)
{
    static unsigned long dsa DEVBSS_ATTR;
    unsigned long addr;
    long offs, size;
    int oldstatus;

    /* read burst dma source address register in channel context */
    sdma_read_words(&dsa, CHANNEL_CONTEXT_ADDR(DMA_PLAY_CH_NUM)+0x0b, 1);

    oldstatus = disable_irq_save();
    addr = dsa;
    offs = addr - (unsigned long)dma_play_bd.buf_addr;
    size = dma_play_bd.mode.count;
    restore_irq(oldstatus);

    /* Be addresses are coherent (no buffer change during read) */
    if (offs >= 0 && offs < size)
    {
        *count = (size - offs) >> 2;
        return (void *)((addr + 2) & ~3);
    }

    *count = 0;
    return NULL;
}

void * pcm_dma_addr(void *addr)
{
    return (void *)addr_virt_to_phys((unsigned long)addr);
}

#ifdef HAVE_RECORDING
static struct buffer_descriptor dma_rec_bd DEVBSS_ATTR;
static struct channel_descriptor dma_rec_cd DEVBSS_ATTR;

static struct dma_data dma_rec_data =
{
    /* Initialize to a locked, stopped state */
    .locked = 0,
    .callback_pending = 0,
    .state = 0
};

static void rec_dma_callback(void)
{
    pcm_more_callback_type2 more_ready;
    int status = 0;

    if (dma_rec_data.locked != 0)
    {
        dma_rec_data.callback_pending = dma_rec_data.state;
        return; /* Callback is locked out */
    }

    if (dma_rec_bd.mode.status & BD_RROR)
        status = DMA_REC_ERROR_DMA;

    more_ready = pcm_callback_more_ready;

    if (more_ready != NULL && more_ready(status) >= 0)
    {
        sdma_channel_run(DMA_REC_CH_NUM);
        return;
    }

    /* Finished recording */
    pcm_rec_dma_stop();
    pcm_rec_dma_stopped_callback();
}

void pcm_rec_lock(void)
{
    if (++dma_rec_data.locked == 1)
        imx31_regclr32(&SSI_SIER2, SSI_SIER_RDMAE);
}

void pcm_rec_unlock(void)
{
    if (--dma_rec_data.locked == 0 && dma_rec_data.state != 0)
    {
        int oldstatus = disable_irq_save();
        int pending = dma_rec_data.callback_pending;
        dma_rec_data.callback_pending = 0;
        SSI_SIER2 |= SSI_SIER_RDMAE;
        restore_irq(oldstatus);

        /* Should an interrupt be forced instead? The upper pcm layer can
         * call consumer's callback in thread context so technically this is
         * acceptable. */
        if (pending != 0)
            rec_dma_callback();
    }
}

void pcm_record_more(void *start, size_t size)
{
    start = (void *)(((unsigned long)start + 3) & ~3);
    size &= ~3;

    /* Invalidate - buffer must be coherent */
    dump_dcache_range(start, size);

    start = (void *)addr_virt_to_phys((unsigned long)start);

    pcm_rec_peak_addr = start;
    dma_rec_bd.buf_addr = start;
    dma_rec_bd.mode.count = size;
    dma_rec_bd.mode.command = TRANSFER_16BIT;
    dma_rec_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;
}

void pcm_rec_dma_stop(void)
{
    /* Stop receiving data */
    sdma_channel_stop(DMA_REC_CH_NUM);

    imx31_regclr32(&SSI_SIER2, SSI_SIER_RDMAE);

    SSI_SCR2 &= ~SSI_SCR_RE;      /* Disable RX */
    SSI_SRCR2 &= ~SSI_SRCR_RFEN0; /* Disable RX FIFO */

    /* Set state before pending to prevent race with interrupt */
    /* Do not enable DMA requests on unlock */
    dma_rec_data.state = 0;
    dma_rec_data.callback_pending = 0;
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    pcm_rec_dma_stop();

    addr = (void *)(((unsigned long)addr + 3) & ~3);
    size &= ~3;

    if (size <= 0)
        return;

    if (!sdma_channel_reset(DMA_REC_CH_NUM))
        return;
    
    /* Invalidate - buffer must be coherent */
    dump_dcache_range(addr, size);

    addr = (void *)addr_virt_to_phys((unsigned long)addr);
    pcm_rec_peak_addr = addr;
    dma_rec_bd.buf_addr = addr;
    dma_rec_bd.mode.count = size;
    dma_rec_bd.mode.command = TRANSFER_16BIT;
    dma_rec_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;

    dma_rec_data.state = 1;

    SSI_SRCR2 |= SSI_SRCR_RFEN0; /* Enable RX FIFO */

    /* Ensure clear FIFO */
    while (SSI_SFCSR2 & SSI_SFCSR_RFCNT0)
        SSI_SRX0_2;

    /* Enable receive */
    SSI_SCR2 |= SSI_SCR_RE;
    sdma_channel_run(DMA_REC_CH_NUM);
}

void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
    sdma_channel_close(DMA_REC_CH_NUM);
}

void pcm_rec_dma_init(void)
{
    pcm_rec_dma_stop();

    /* Init channel information */
    dma_rec_cd.bd_count = 1;
    dma_rec_cd.callback = rec_dma_callback;
    dma_rec_cd.shp_addr = SDMA_PER_ADDR_SSI2_RX1;
    dma_rec_cd.wml      = SDMA_SSI_RXFIFO_WML*2;
    dma_rec_cd.per_type = SDMA_PER_SSI;
    dma_rec_cd.tran_type = SDMA_TRAN_PER_2_EMI;
    dma_rec_cd.event_id1 = SDMA_REQ_SSI2_RX1;

    sdma_channel_init(DMA_REC_CH_NUM, &dma_rec_cd, &dma_rec_bd);
}

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    static unsigned long pda DEVBSS_ATTR;
    unsigned long buf, addr, end, bufend;
    int oldstatus;

    /* read burst dma destination address register in channel context */
    sdma_read_words(&pda, CHANNEL_CONTEXT_ADDR(DMA_REC_CH_NUM)+0x0a, 1);

    oldstatus = disable_irq_save();
    end = pda;
    buf = (unsigned long)dma_rec_bd.buf_addr;
    addr = (unsigned long)pcm_rec_peak_addr;
    bufend = buf + dma_rec_bd.mode.count;
    restore_irq(oldstatus);

    /* Be addresses are coherent (no buffer change during read) */
    if (addr >= buf && addr < bufend &&
        end >= buf && end < bufend)
    {
        *count = (end >> 2) - (addr >> 2);
        return (void *)(addr & ~3);
    }

    *count = 0;
    return NULL;
}

#endif /* HAVE_RECORDING */
