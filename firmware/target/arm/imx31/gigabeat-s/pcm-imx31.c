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
#include "avic-imx31.h"
#include "clkctl-imx31.h"

/* This isn't DMA-based at the moment and is handled like Portal Player but
 * will suffice for starters. */

struct dma_data
{
    uint16_t *p;
    size_t size;
    int locked;
    int state;
};

static unsigned long pcm_freq; /* 44.1 is default */
static int sr_ctrl;

static struct dma_data dma_play_data =
{
    /* Initialize to a locked, stopped state */
    .p = NULL,
    .size = 0,
    .locked = 0,
    .state = 0
};

void pcm_play_lock(void)
{
    if (++dma_play_data.locked == 1)
    {
        /* Atomically disable transmit interrupt */
        imx31_regmod32(&SSI_SIER1, 0, SSI_SIER_TIE);
    }
}

void pcm_play_unlock(void)
{
    if (--dma_play_data.locked == 0 && dma_play_data.state != 0)
    {
        /* Atomically enable transmit interrupt */
        imx31_regmod32(&SSI_SIER1, SSI_SIER_TIE, SSI_SIER_TIE);
    }
}

static void _pcm_apply_settings(void)
{
    if (pcm_freq != pcm_curr_sampr)
    {
        pcm_curr_sampr = pcm_freq;
        audiohw_set_frequency(sr_ctrl);
    }
}

static void __attribute__((interrupt("IRQ"))) SSI1_HANDLER(void)
{
    register pcm_more_callback_type get_more;

    do
    {
        while (dma_play_data.size > 0)
        {
            if (SSI_SFCSR_TFCNT0r(SSI_SFCSR1) > 6)
            {
                return;
            }
            SSI_STX0_1 = *dma_play_data.p++;
            SSI_STX0_1 = *dma_play_data.p++;
            dma_play_data.size -= 4;
        }

        /* p is empty, get some more data */
        get_more = pcm_callback_for_more;

        if (get_more)
        {
            get_more((unsigned char **)&dma_play_data.p,
                     &dma_play_data.size);
        }
    }
    while (dma_play_data.size > 0);

    /* No more data, so disable the FIFO/interrupt */
    pcm_play_dma_stop();
    pcm_play_dma_stopped_callback();
}

void pcm_apply_settings(void)
{
    pcm_play_lock();
#ifdef HAVE_RECORDING
    pcm_rec_lock();
#endif

    _pcm_apply_settings();

#ifdef HAVE_RECORDING
    pcm_rec_unlock();
#endif
    pcm_play_unlock();
}

void pcm_play_dma_init(void)
{
    imx31_clkctl_module_clock_gating(CG_SSI1, CGM_ON_ALL);
    imx31_clkctl_module_clock_gating(CG_SSI2, CGM_ON_ALL);

    /* Reset & disable SSIs */
    SSI_SCR2 &= ~SSI_SCR_SSIEN;
    SSI_SCR1 &= ~SSI_SCR_SSIEN;

    SSI_SIER1 = SSI_SIER_TFE0;
    SSI_SIER2 = 0;

    /* Set up audio mux */

    /* Port 1 (internally connected to SSI1)
     * All clocking is output sourced from port 4 */
    AUDMUX_PTCR1 = AUDMUX_PTCR_TFS_DIR | AUDMUX_PTCR_TFSEL_PORT4 |
                   AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT4 |
                   AUDMUX_PTCR_RFSDIR  | AUDMUX_PTCR_RFSSEL_PORT4 |
                   AUDMUX_PTCR_RCLKDIR | AUDMUX_PTCR_RCSEL_PORT4 |
                   AUDMUX_PTCR_SYN;
 
    /* Receive data from port 4 */
    AUDMUX_PDCR1 = AUDMUX_PDCR_RXDSEL_PORT4;
    /* All clock lines are inputs sourced from the master mode codec and
     * sent back to SSI1 through port 1 */
    AUDMUX_PTCR4 = AUDMUX_PTCR_SYN;

    /* Receive data from port 1 */
    AUDMUX_PDCR4 = AUDMUX_PDCR_RXDSEL_PORT1;

    /* Port 2 (internally connected to SSI2) routes clocking to port 5 to
     * provide MCLK to the codec */
    /* All port 2 clocks are inputs taken from SSI2 */
    AUDMUX_PTCR2 = 0;
    AUDMUX_PDCR2 = 0;
    /* Port 5 outputs TCLK sourced from port 2 */
    AUDMUX_PTCR5 = AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT2;
    AUDMUX_PDCR5 = 0;

    /* Setup SSIs */

    /* SSI1 - interface for all I2S data */
    SSI_SCR1 = SSI_SCR_SYN | SSI_SCR_I2S_MODE_SLAVE;
    SSI_STCR1 = SSI_STCR_TXBIT0 | SSI_STCR_TSCKP | SSI_STCR_TFSI |
                SSI_STCR_TEFS | SSI_STCR_TFEN0;

    /* 16 bits per word, 2 words per frame */
    SSI_STCCR1 = SSI_STRCCR_WL16 | SSI_STRCCR_DCw(2-1) |
                 SSI_STRCCR_PMw(4-1);

    SSI_STMSK1 = 0;

    /* Receive */
    SSI_SRCR1 = SSI_SRCR_RXBIT0 | SSI_SRCR_RSCKP | SSI_SRCR_RFSI |
                SSI_SRCR_REFS | SSI_SRCR_RFEN0;

    /* 16 bits per word, 2 words per frame */
    SSI_SRCCR1 = SSI_STRCCR_WL16 | SSI_STRCCR_DCw(2-1) |
                 SSI_STRCCR_PMw(4-1);

    /* Receive high watermark - 6 samples in FIFO
     * Transmit low watermark - 2 samples in FIFO */
    SSI_SFCSR1 = SSI_SFCSR_RFWM1w(8) | SSI_SFCSR_TFWM1w(1) |
                 SSI_SFCSR_RFWM0w(6) | SSI_SFCSR_TFWM0w(2);

    SSI_SRMSK1 = 0;

    /* SSI2 - provides MCLK only */
    SSI_SCR2 = 0;
    SSI_SRCR2 = 0;
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

    /* Enable SSI2 (codec clock) */
    SSI_SCR2 |= SSI_SCR_SSIEN;

    pcm_set_frequency(HW_SAMPR_DEFAULT);
    audiohw_init();
}

void pcm_postinit(void)
{
    audiohw_postinit();
    avic_enable_int(SSI1, IRQ, 8, SSI1_HANDLER);
}

static void play_start_pcm(void)
{
    /* Stop transmission (if in progress) */
    SSI_SCR1 &= ~SSI_SCR_TE;

    /* Apply new settings */
    _pcm_apply_settings();

    /* Enable interrupt on unlock */
    dma_play_data.state = 1;

    /* Fill the FIFO or start when data is used up */
    SSI_SCR1 |= SSI_SCR_SSIEN; /* Enable SSI */

    while (1)
    {
        if (SSI_SFCSR_TFCNT0r(SSI_SFCSR1) > 6 || dma_play_data.size == 0)
        {
            SSI_SCR1 |= SSI_SCR_TE;  /* Start transmitting */
            return;
        }

        SSI_STX0_1 = *dma_play_data.p++;
        SSI_STX0_1 = *dma_play_data.p++;
        dma_play_data.size -= 4;
    }
}

static void play_stop_pcm(void)
{
    /* Disable interrupt */
    SSI_SIER1 &= ~SSI_SIER_TIE;

    /* Wait for FIFO to empty */
    while (SSI_SFCSR_TFCNT0r(SSI_SFCSR1) > 0);

    /* Disable transmission */
    SSI_SCR1 &= ~(SSI_SCR_TE | SSI_SCR_SSIEN);

    /* Do not enable interrupt on unlock */
    dma_play_data.state = 0;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_play_data.p    = (void *)(((uintptr_t)addr + 3) & ~3);
    dma_play_data.size = (size & ~3);

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    play_stop_pcm();
    dma_play_data.size = 0;
}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        play_stop_pcm();
    }
    else
    {
        uint32_t addr = (uint32_t)dma_play_data.p;
        dma_play_data.p = (void *)((addr + 2) & ~3);
        dma_play_data.size &= ~3;
        play_start_pcm();
    }
}

/* Set the pcm frequency hardware will use when play is next started or
   when pcm_apply_settings is called. Do not apply the setting to the
   hardware here but simply cache it. */
void pcm_set_frequency(unsigned int frequency)
{
    int index;

    switch (frequency)
    {
    case SAMPR_48:
        index = HW_FREQ_48;
        break;
    case SAMPR_44:
        index = HW_FREQ_44;
        break;
    case SAMPR_32:
        index = HW_FREQ_32;
        break;
    case SAMPR_24:
        index = HW_FREQ_24;
        break;
    case SAMPR_22:
        index = HW_FREQ_22;
        break;
    case SAMPR_16:
        index = HW_FREQ_16;
        break;
    case SAMPR_12:
        index = HW_FREQ_12;
        break;
    case SAMPR_11:
        index = HW_FREQ_11;
        break;
    case SAMPR_8:
        index = HW_FREQ_8;
        break;
    default:
        /* Invalid = default */
        frequency = HW_SAMPR_DEFAULT;
        index = HW_FREQ_DEFAULT;
    }

    pcm_freq = frequency;
    sr_ctrl = index;
}

/* Return the number of bytes waiting - full L-R sample pairs only */
size_t pcm_get_bytes_waiting(void)
{
    return dma_play_data.size & ~3;
}

/* Return a pointer to the samples and the number of them in *count */
const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uint32_t addr = (uint32_t)dma_play_data.p;
    size_t cnt = dma_play_data.size;
    *count = cnt >> 2;
    return (void *)((addr + 2) & ~3);
}

/* Any recording functionality should be implemented similarly */
