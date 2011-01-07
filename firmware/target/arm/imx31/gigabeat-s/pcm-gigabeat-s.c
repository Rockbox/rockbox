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
#include "ccm-imx31.h"
#include "sdma-imx31.h"
#include "mmu-imx31.h"

#define DMA_PLAY_CH_NUM 2
#define DMA_REC_CH_NUM 1
#define DMA_PLAY_CH_PRIORITY 6
#define DMA_REC_CH_PRIORITY 6

static struct buffer_descriptor dma_play_bd NOCACHEBSS_ATTR;

static void play_dma_callback(void);
static struct channel_descriptor dma_play_cd =
{
    .bd_count = 1,
    .callback = play_dma_callback,
    .shp_addr = SDMA_PER_ADDR_SSI2_TX1,
    .wml      = SDMA_SSI_TXFIFO_WML*2,
    .per_type = SDMA_PER_SSI_SHP, /* SSI2 shared with SDMA core */
    .tran_type = SDMA_TRAN_EMI_2_PER,
    .event_id1 = SDMA_REQ_SSI2_TX1,
};

/* The pcm locking relies on the fact the interrupt handlers run to completion
 * before lower-priority modes proceed. We don't have to touch hardware
 * registers. Disabling SDMA interrupt would disable DMA callbacks systemwide
 * and that is not something that is desireable.
 *
 * Lock explanation [++.locked]:
 * Trivial, just increment .locked.
 *
 * Unlock explanation [if (--.locked == 0 && .state != 0)]:
 * If int occurred and saw .locked as nonzero, we'll get a pending
 * and it will have taken no action other than to set the flag to the
 * value of .state. If it saw zero for .locked, it will have proceeded
 * normally into the pcm callbacks. If cb set the pending flag, it has
 * to be called to kickstart the callback mechanism and DMA. If the unlock
 * came after a stop, we won't be in the block and DMA will be off. If
 * we're still doing transfers, cb will see 0 for .locked and if pending,
 * it won't be called by DMA again. */
struct dma_data
{
    int locked;
    int callback_pending; /* DMA interrupt happened while locked */
    int state;
};

static struct dma_data dma_play_data =
{
    /* Initialize to an unlocked, stopped state */
    .locked = 0,
    .callback_pending = 0,
    .state = 0
};

static void play_dma_callback(void)
{
    void *start;
    size_t size;
    bool rror;

    if (dma_play_data.locked != 0)
    {
        /* Callback is locked out */
        dma_play_data.callback_pending = dma_play_data.state;
        return;
    }

    rror = dma_play_bd.mode.status & BD_RROR;

    pcm_play_get_more_callback(rror ? NULL : &start, &size);

    if (size == 0)
        return;

     /* Flush any pending cache writes */
    clean_dcache_range(start, size);
    dma_play_bd.buf_addr = (void *)addr_virt_to_phys((unsigned long)start);
    dma_play_bd.mode.count = size;
    dma_play_bd.mode.command = TRANSFER_16BIT;
    dma_play_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;
    sdma_channel_run(DMA_PLAY_CH_NUM);
}

void pcm_play_lock(void)
{
    /* Need to prevent DVFS from causing interrupt priority inversion if audio
     * is locked and a DVFS interrupt fires, blocking reenabling of audio by a 
     * low-priority mode for at least the duration of the lengthy DVFS routine.
     * Not really an issue with state changes but lockout when playing.
     *
     * Keep direct use of DVFS code away from here though. This could provide
     * more services in the future anyway. */
    kernel_audio_locking(true);
    ++dma_play_data.locked;
}

void pcm_play_unlock(void)
{
    if (--dma_play_data.locked == 0)
    {
        if (dma_play_data.state != 0)
        {
            int oldstatus = disable_irq_save();
            int pending = dma_play_data.callback_pending;
            dma_play_data.callback_pending = 0;
            restore_irq(oldstatus);

            if (pending != 0)
                play_dma_callback();
        }

        kernel_audio_locking(false);
    }
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

void pcm_play_dma_init(void)
{
    /* Init channel information */
    sdma_channel_init(DMA_PLAY_CH_NUM, &dma_play_cd, &dma_play_bd);
    sdma_channel_set_priority(DMA_PLAY_CH_NUM, DMA_PLAY_CH_PRIORITY);

    ccm_module_clock_gating(CG_SSI1, CGM_ON_RUN_WAIT);
    ccm_module_clock_gating(CG_SSI2, CGM_ON_RUN_WAIT);

    /* Reset & disable SSIs */
    SSI_SCR1 &= ~SSI_SCR_SSIEN;
    SSI_SCR2 &= ~SSI_SCR_SSIEN;

    SSI_SIER1 = 0;
    SSI_SIER2 = 0;

    /* Set up audio mux */

    /* Port 2 (internally connected to SSI2)
     * All clocking is output sourced from port 4 */
    AUDMUX_PTCR2 = AUDMUX_PTCR_TFS_DIR | AUDMUX_PTCR_TFSEL_PORT4 |
                   AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT4 |
                   AUDMUX_PTCR_SYN;
 
    /* Receive data from port 4 */
    AUDMUX_PDCR2 = AUDMUX_PDCR_RXDSEL_PORT4;
    /* All clock lines are inputs sourced from the master mode codec and
     * sent back to SSI2 through port 2 */
    AUDMUX_PTCR4 = AUDMUX_PTCR_SYN;

    /* Receive data from port 2 */
    AUDMUX_PDCR4 = AUDMUX_PDCR_RXDSEL_PORT2;

    /* PORT1 (internally connected to SSI1) routes clocking to PORT5 to
     * provide MCLK to the codec */
    /* TX clocks are inputs taken from SSI2 */
    /* RX clocks are outputs taken from PORT4 */
    AUDMUX_PTCR1 = AUDMUX_PTCR_RFS_DIR | AUDMUX_PTCR_RFSSEL_PORT4 |
                   AUDMUX_PTCR_RCLKDIR | AUDMUX_PTCR_RCSEL_PORT4;
    /* RX data taken from PORT4 */
    AUDMUX_PDCR1 = AUDMUX_PDCR_RXDSEL_PORT4;

    /* PORT5 outputs TCLK sourced from PORT1 (SSI1) */
    AUDMUX_PTCR5 = AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT1;
    AUDMUX_PDCR5 = 0;

    /* Setup SSIs */

    /* SSI2 - SoC software interface for all I2S data out */
    SSI_SCR2 = SSI_SCR_SYN | SSI_SCR_I2S_MODE_SLAVE;
    SSI_STCR2 = SSI_STCR_TXBIT0 | SSI_STCR_TSCKP | SSI_STCR_TFSI |
                SSI_STCR_TEFS | SSI_STCR_TFEN0;

    /* 16 bits per word, 2 words per frame */
    SSI_STCCR2 = SSI_STRCCR_WL16 | ((2-1) << SSI_STRCCR_DC_POS) |
                 ((4-1) << SSI_STRCCR_PM_POS);

    /* Transmit low watermark */
    SSI_SFCSR2 = (SSI_SFCSR2 & ~SSI_SFCSR_TFWM0) |
                 ((8-SDMA_SSI_TXFIFO_WML) << SSI_SFCSR_TFWM0_POS);
    SSI_STMSK2 = 0;

    /* SSI1 - provides MCLK to codec. Receives data from codec. */
    SSI_STCR1 = SSI_STCR_TXDIR;

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
    SSI_STCCR1 = SSI_STRCCR_DIV2 | ((1-1) << SSI_STRCCR_PM_POS);

    /* SSI1 - receive - asynchronous clocks */
    SSI_SCR1 = SSI_SCR_I2S_MODE_SLAVE;

    SSI_SRCR1 = SSI_SRCR_RXBIT0 | SSI_SRCR_RSCKP | SSI_SRCR_RFSI |
                SSI_SRCR_REFS;

    /* 16 bits per word, 2 words per frame */
    SSI_SRCCR1 = SSI_STRCCR_WL16 | ((2-1) << SSI_STRCCR_DC_POS) |
                 ((4-1) << SSI_STRCCR_PM_POS);

    /* Receive high watermark */
    SSI_SFCSR1 = (SSI_SFCSR1 & ~SSI_SFCSR_RFWM0) |
                 (SDMA_SSI_RXFIFO_WML << SSI_SFCSR_RFWM0_POS);
    SSI_SRMSK1 = 0;

    /* Enable SSI1 (codec clock) */
    SSI_SCR1 |= SSI_SCR_SSIEN;

    audiohw_init();
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

static void play_start_pcm(void)
{
    /* Stop transmission (if in progress) */
    SSI_SCR2 &= ~SSI_SCR_TE;

    SSI_SCR2 |= SSI_SCR_SSIEN;   /* Enable SSI */
    SSI_STCR2 |= SSI_STCR_TFEN0; /* Enable TX FIFO */

    dma_play_data.state = 1; /* Check callback on unlock */

    /* Do prefill to prevent swapped channels (see TLSbo61214 in MCIMX31CE).
     * No actual solution was offered but this appears to work. */
    SSI_STX0_2 = 0;
    SSI_STX0_2 = 0;
    SSI_STX0_2 = 0;
    SSI_STX0_2 = 0;

    SSI_SIER2 |= SSI_SIER_TDMAE; /* Enable DMA req. */
    SSI_SCR2 |= SSI_SCR_TE;      /* Start transmitting */
}

static void play_stop_pcm(void)
{
    SSI_SIER2 &= ~SSI_SIER_TDMAE; /* Disable DMA req. */

    /* Set state before pending to prevent race with interrupt */
    dma_play_data.state = 0;

    /* Wait for FIFO to empty */
    while (SSI_SFCSR_TFCNT0 & SSI_SFCSR2);

    SSI_STCR2 &= ~SSI_STCR_TFEN0; /* Disable TX */
    SSI_SCR2 &= ~(SSI_SCR_TE | SSI_SCR_SSIEN); /* Disable transmission, SSI */

    if (pcm_playing)
    {
        /* Stopping: clear buffer info to ensure 0-size readbacks when
         * stopped */
        unsigned long dsa = 0;
        dma_play_bd.buf_addr = NULL;
        dma_play_bd.mode.count = 0;
        clean_dcache_range(&dsa, sizeof(dsa));
        sdma_write_words(&dsa, CHANNEL_CONTEXT_ADDR(DMA_PLAY_CH_NUM)+0x0b, 1);
    }

    /* Clear any pending callback */
    dma_play_data.callback_pending = 0;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    sdma_channel_stop(DMA_PLAY_CH_NUM);

    /* Disable transmission */
    SSI_STCR2 &= ~SSI_STCR_TFEN0;
    SSI_SCR2 &= ~(SSI_SCR_TE | SSI_SCR_SSIEN);

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
    static unsigned long dsa NOCACHEBSS_ATTR;
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
    static unsigned long dsa NOCACHEBSS_ATTR;
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
static struct buffer_descriptor dma_rec_bd NOCACHEBSS_ATTR;

static void rec_dma_callback(void);
static struct channel_descriptor dma_rec_cd =
{
    .bd_count = 1,
    .callback = rec_dma_callback,
    .shp_addr = SDMA_PER_ADDR_SSI1_RX1,
    .wml      = SDMA_SSI_RXFIFO_WML*2,
    .per_type = SDMA_PER_SSI,
    .tran_type = SDMA_TRAN_PER_2_EMI,
    .event_id1 = SDMA_REQ_SSI1_RX1,
};

static struct dma_data dma_rec_data =
{
    /* Initialize to an unlocked, stopped state */
    .locked = 0,
    .callback_pending = 0,
    .state = 0
};

static void rec_dma_callback(void)
{
    int status = 0;
    void *start;
    size_t size;

    if (dma_rec_data.locked != 0)
    {
        dma_rec_data.callback_pending = dma_rec_data.state;
        return; /* Callback is locked out */
    }

    if (dma_rec_bd.mode.status & BD_RROR)
        status = DMA_REC_ERROR_DMA;

    pcm_rec_more_ready_callback(status, &start, &size);

    if (size == 0)
        return;

    /* Invalidate - buffer must be coherent */
    dump_dcache_range(start, size);

    start = (void *)addr_virt_to_phys((unsigned long)start);

    dma_rec_bd.buf_addr = start;
    dma_rec_bd.mode.count = size;
    dma_rec_bd.mode.command = TRANSFER_16BIT;
    dma_rec_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;

    sdma_channel_run(DMA_REC_CH_NUM);
}

void pcm_rec_lock(void)
{
    kernel_audio_locking(true);    
    ++dma_rec_data.locked;
}

void pcm_rec_unlock(void)
{
    if (--dma_rec_data.locked == 0)
    {
        if (dma_rec_data.state != 0)
        {
            int oldstatus = disable_irq_save();
            int pending = dma_rec_data.callback_pending;
            dma_rec_data.callback_pending = 0;
            restore_irq(oldstatus);

            if (pending != 0)
                rec_dma_callback();
        }

        kernel_audio_locking(false);
    }
}

void pcm_rec_dma_stop(void)
{
    SSI_SIER1 &= ~SSI_SIER_RDMAE; /* Disable DMA req. */

    /* Set state before pending to prevent race with interrupt */
    dma_rec_data.state = 0;

    /* Stop receiving data */
    sdma_channel_stop(DMA_REC_CH_NUM);

    bitclr32(&SSI_SIER1, SSI_SIER_RDMAE);

    SSI_SCR1 &= ~SSI_SCR_RE;      /* Disable RX */
    SSI_SRCR1 &= ~SSI_SRCR_RFEN0; /* Disable RX FIFO */

    if (pcm_recording)
    {
        /* Stopping: clear buffer info to ensure 0-size readbacks when
         * stopped */
        unsigned long pda = 0;
        dma_rec_bd.buf_addr = NULL;
        dma_rec_bd.mode.count = 0;
        clean_dcache_range(&pda, sizeof(pda));
        sdma_write_words(&pda, CHANNEL_CONTEXT_ADDR(DMA_REC_CH_NUM)+0x0a, 1);
    }

    /* Clear any pending callback */
    dma_rec_data.callback_pending = 0;
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    pcm_rec_dma_stop();

    if (!sdma_channel_reset(DMA_REC_CH_NUM))
        return;
    
    /* Invalidate - buffer must be coherent */
    dump_dcache_range(addr, size);

    addr = (void *)addr_virt_to_phys((unsigned long)addr);
    dma_rec_bd.buf_addr = addr;
    dma_rec_bd.mode.count = size;
    dma_rec_bd.mode.command = TRANSFER_16BIT;
    dma_rec_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;

    dma_rec_data.state = 1; /* Check callback on unlock */

    SSI_SRCR1 |= SSI_SRCR_RFEN0; /* Enable RX FIFO */

    /* Ensure clear FIFO */
    while (SSI_SFCSR1 & SSI_SFCSR_RFCNT0)
        SSI_SRX0_1;

    /* Enable receive */
    SSI_SCR1 |= SSI_SCR_RE;
    SSI_SIER1 |= SSI_SIER_RDMAE; /* Enable DMA req. */

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
    sdma_channel_init(DMA_REC_CH_NUM, &dma_rec_cd, &dma_rec_bd);
    sdma_channel_set_priority(DMA_REC_CH_NUM, DMA_REC_CH_PRIORITY);
}

const void * pcm_rec_dma_get_peak_buffer(void)
{
    static unsigned long pda NOCACHEBSS_ATTR;
    unsigned long buf, end, bufend;
    int oldstatus;

    /* read burst dma destination address register in channel context */
    sdma_read_words(&pda, CHANNEL_CONTEXT_ADDR(DMA_REC_CH_NUM)+0x0a, 1);

    oldstatus = disable_irq_save();
    end = pda;
    buf = (unsigned long)dma_rec_bd.buf_addr;
    bufend = buf + dma_rec_bd.mode.count;
    restore_irq(oldstatus);

    /* Be addresses are coherent (no buffer change during read) */
    if (end >= buf && end < bufend)
        return (void *)(end & ~3);

    return NULL;
}

#endif /* HAVE_RECORDING */
