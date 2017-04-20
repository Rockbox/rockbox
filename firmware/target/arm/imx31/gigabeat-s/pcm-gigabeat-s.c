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
#include "sdma-imx31.h"
#include "mmu-imx31.h"
#include "pcm-internal.h"

#define DMA_PLAY_CH_NUM 2
#define DMA_REC_CH_NUM 1
#define DMA_PLAY_CH_PRIORITY 6
#define DMA_REC_CH_PRIORITY 6

#define PLAY_MSA_ADDR (CHANNEL_CONTEXT_ADDR(DMA_PLAY_CH_NUM)+0x0b)
#define REC_MDA_ADDR  (CHANNEL_CONTEXT_ADDR(DMA_REC_CH_NUM)+0x0a)

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
 * Lock explanation [DMA_INT_LOCKED:1]:
 * Trivial, just set DMA_INT_LOCKED:1
 *
 * Unlock explanation [DMA_INT_LOCKED:0 && DMA_INT_ON:1]:
 * If int occurred and saw DMA_INT_LOCKED:1, callback will set DMA_CB_PENDING:1
 * and it will have taken no action other than to set that flag. If it saw
 * DMA_INT_LOCKED:0, it will have proceeded normally into the pcm callbacks.
 * If cb set DMA_CB_PENDING:1, it has to be called when releasing the lockout
 * in order to kickstart the callback mechanism and DMA if DMA_INT_ON:1. If an
 * automatic stop was due, it will also complete normally when kickstarting.
 * Another interrupt can't happen unless the channel was explicitly restarted.
 */
#define DMA_INT_LOCKED 0x1ul
#define DMA_INT_ON     0x2ul
#define DMA_CB_PENDING 0x4ul

static unsigned long dma_play_state;

static void play_dma_callback(void)
{
    if (dma_play_state & DMA_INT_LOCKED)
    {
        dma_play_state |= DMA_CB_PENDING;
        return; /* Callback is locked out */
    }

    pcm_play_dma_complete_callback((dma_play_bd.mode.status & BD_RROR) ? -EIO : 0);
}

void pcm_play_dma_lock(void)
{
    bitset32(&dma_play_state, DMA_INT_LOCKED);
}

void pcm_play_dma_unlock(void)
{
    unsigned long state =
        bitclr32(&dma_play_state, DMA_INT_LOCKED|DMA_CB_PENDING);

    if ((state & (DMA_INT_ON|DMA_CB_PENDING)) == (DMA_INT_ON|DMA_CB_PENDING))
        play_dma_callback();
}

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

void pcm_play_dma_init(void)
{
    /* Init DMA channel information */
    sdma_channel_init(DMA_PLAY_CH_NUM, &dma_play_cd, &dma_play_bd);
    sdma_channel_set_priority(DMA_PLAY_CH_NUM, DMA_PLAY_CH_PRIORITY);

    /* Init audio interfaces */
    audiohw_init();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

static NO_INLINE void tx_start(void)
{
    SSI_SCR2 &= ~SSI_SCR_TE;     /* Stop transmission (if going) */
    SSI_SCR2 |= SSI_SCR_SSIEN;   /* Enable SSI */
    SSI_STCR2 |= SSI_STCR_TFEN0; /* Enable TX FIFO */

    bitset32(&dma_play_state, DMA_INT_ON); /* Check callback on unlock */

    /* Do prefill to prevent swapped channels (see TLSbo61214 in MCIMX31CE).
     * No actual solution was offered but this appears to work. */
    while ((SSI_SFCSR2 & SSI_SFCSR_TFCNT0) < (8 << SSI_SFCSR_TFCNT0_POS))
        SSI_STX0_2 = 0;

    SSI_SIER2 |= SSI_SIER_TDMAE; /* Enable DMA req. */
    SSI_SCR2 |= SSI_SCR_TE;      /* Start transmitting */
}

static NO_INLINE void tx_stop(void)
{
    SSI_SIER2 &= ~SSI_SIER_TDMAE; /* Disable DMA req. */

    /* Set state before pending to prevent race with interrupt */
    bool int_on = bitclr32(&dma_play_state, DMA_INT_ON) & DMA_INT_ON;

    if (int_on)
        while (SSI_SFCSR2 & SSI_SFCSR_TFCNT0); /* Wait for FIFO to empty */

    SSI_STCR2 &= ~SSI_STCR_TFEN0;              /* Disable TX */
    SSI_SCR2 &= ~(SSI_SCR_TE | SSI_SCR_SSIEN); /* Disable transmission, SSI */

    if (int_on)
    {
        /* Stopping: clear buffer info to ensure 0-size readbacks when
         * stopped */
        dma_play_bd.buf_addr = NULL;
        dma_play_bd.mode.count = 0;
        sdma_write_word(PLAY_MSA_ADDR, 0);
    }

    bitclr32(&dma_play_state, DMA_CB_PENDING); /* Clear any pending callback */
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    size_t size = frames*4;
    commit_dcache_range(addr, size);

    dma_play_bd.buf_addr = (void *)addr_virt_to_phys((unsigned long)addr);
    dma_play_bd.mode.count = size;
    dma_play_bd.mode.command = TRANSFER_16BIT;
    dma_play_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;

    if (!(dma_play_state & DMA_INT_ON))
        tx_start();

    sdma_channel_run(DMA_PLAY_CH_NUM);
}

void pcm_play_dma_prepare(void)
{
    pcm_play_dma_stop();
    sdma_channel_reset(DMA_PLAY_CH_NUM);
}

void pcm_play_dma_stop(void)
{
    sdma_channel_stop(DMA_PLAY_CH_NUM);
    tx_stop();
}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        sdma_channel_pause(DMA_PLAY_CH_NUM);
        tx_stop();
    }
    else
    {
        tx_start();
        sdma_channel_run(DMA_PLAY_CH_NUM);
    }
}

/* Return the number of frames remaining - full L-R sample pairs only */
unsigned long pcm_get_frames_waiting(void)
{
    /* read burst dma source address register in channel context */
    unsigned long msa = sdma_read_word(PLAY_MSA_ADDR);

    unsigned long oldstatus = disable_irq_save();
    unsigned long buf = (unsigned long)dma_play_bd.buf_addr;
    unsigned long bufend = buf + dma_play_bd.mode.count;
    restore_irq(oldstatus);

    /* Be addresses are coherent (no buffer change during read) */
    if (msa >= buf && msa < bufend)
        return (bufend - msa) / 4;

    return 0;
}

/* Return a pointer to the frames and the number of them in *frames_rem */
const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    /* read burst dma source address register in channel context */
    unsigned long msa = sdma_read_word(PLAY_MSA_ADDR);

    unsigned long oldstatus = disable_irq_save();
    unsigned long buf = (unsigned long)dma_play_bd.buf_addr;
    unsigned long bufend = buf + dma_play_bd.mode.count;
    restore_irq(oldstatus);

    /* Be addresses are coherent (no buffer change during read) */
    if (msa >= buf && msa < bufend)
    {
        *frames_rem = (bufend - msa) / 4;
        return (void *)(msa & ~3);
    }

    *frames_rem = 0;
    return NULL;
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

static unsigned long dma_data dma_rec_state;

static void rec_dma_callback(void)
{
    if (dma_rec_state & DMA_INT_LOCKED)
    {
        dma_rec_state |= DMA_CB_PENDING;
        return; /* Callback is locked out */
    }

    pcm_rec_dma_complete_callback((dma_rec_bd.mode.status & BD_RROR) ? -EIO : 0);
}

static NO_INLINE void rx_start(void)
{
    SSI_SRCR1 |= SSI_SRCR_RFEN0;  /* Enable RX FIFO */

    while (SSI_SFCSR1 & SSI_SFCSR_RFCNT0) /* Ensure clear FIFO */
        SSI_SRX0_1;

    SSI_SCR1 |= SSI_SCR_RE;       /* Enable receive */
    SSI_SIER1 |= SSI_SIER_RDMAE;  /* Enable DMA req. */

    bitset32(&dma_rec_state, DMA_INT_ON); /* Check callback on unlock */
}

static NO_INLINE void rx_stop(void)
{
    SSI_SIER1 &= ~SSI_SIER_RDMAE; /* Disable DMA req. */

    /* Set state before pending to prevent race with interrupt */
    bool int_on = bitclr32(&dma_rec_state, DMA_INT_ON) & DMA_INT_ON;

    SSI_SCR1 &= ~SSI_SCR_RE;      /* Disable RX */
    SSI_SRCR1 &= ~SSI_SRCR_RFEN0; /* Disable RX FIFO */

    /* SSI1 provides the clocking signals for the codec and is always ON */

    if (int_on)
    {
        /* Stopping: clear buffer info to ensure 0-size readbacks when
         * stopped */
        dma_rec_bd.buf_addr = NULL;
        dma_rec_bd.mode.count = 0;
        sdma_write_word(REC_MDA_ADDR, 0);
    }

    bitclr32(&dma_rec_state, DMA_CB_PENDING); /* Clear any pending callback */
}

void pcm_rec_dma_lock(void)
{
    bitset32(&dma_rec_state, DMA_INT_LOCKED);
}

void pcm_rec_dma_unlock(void)
{
    unsigned long state =
        bitclr32(&dma_rec_state, DMA_INT_LOCKED|DMA_CB_PENDING);

    if ((state & (DMA_INT_ON|DMA_CB_PENDING)) == (DMA_INT_ON|DMA_CB_PENDING))
        rec_dma_callback();
}

void pcm_rec_dma_stop(void)
{
    sdma_channel_stop(DMA_REC_CH_NUM);
    rx_stop();
}

void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    size_t size = frames*4;
    discard_dcache_range(addr, size);

    addr = (void *)addr_virt_to_phys((unsigned long)addr);

    dma_rec_bd.buf_addr = addr;
    dma_rec_bd.mode.count = size;
    dma_rec_bd.mode.command = TRANSFER_16BIT;
    dma_rec_bd.mode.status = BD_DONE | BD_WRAP | BD_INTR;

    if (!(dma_rec_state & DMA_INT_ON))
        rx_start();

    sdma_channel_run(DMA_REC_CH_NUM);
}

void pcm_rec_dma_prepare(void)
{
    pcm_rec_dma_stop();
    sdma_channel_reset(DMA_REC_CH_NUM);
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

const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    /* read burst dma destination address register in channel context */
    unsigned long mda = sdma_read_word(REC_MDA_ADDR) & ~3;

    unsigned long oldstatus = disable_irq_save();
    unsigned long buf = (unsigned long)dma_rec_bd.buf_addr;
    unsigned long bufend = buf + dma_rec_bd.mode.count;
    restore_irq(oldstatus);

    /* Be sure addresses are coherent (no buffer change during read) */
    if (mda >= buf && mda < bufend)
    {
        *frames_avail = (mda - buf) / 4;
        return (void *)buf;
    }

    *frames_avail = 0;
    return NULL;
}

#endif /* HAVE_RECORDING */
