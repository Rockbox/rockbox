/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
 * Copyright (C) 2008 by Rob Purchase
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
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "i2s.h"
#include "pcm-internal.h"

struct dma_data
{
/* NOTE: The order of size and p is important if you use assembler
   optimised fiq handler, so don't change it. */
    union
    {
        int16_t *p;
        const void *p_r;
        void *p_w;
    };
    unsigned long frames;
    unsigned long state;
};

/****************************************************************************
 ** Playback DMA transfer
 **/
struct dma_data dma_play_data SHAREDBSS_ATTR =
{
    /* Initialize to a locked, stopped state */
    { .p = NULL },
    .frames = 0,
    .state = 0
};

const void * pcm_play_dma_get_peak_buffer(unsigned long *frames_rem)
{
    int oldstatus = disable_irq_save();
    unsigned long addr = (unsigned long)dma_play_data.p_r;
    unsigned long frames = dma_play_data.frames;
    restore_irq(oldstatus);

    *frames_rem = frames;
    return (void *)((addr + 2) & ~3);
}

void pcm_dma_init(const struct pcm_hw_settings *settings)
{
    DAVC = 0x0;         /* Digital Volume = max */
#ifdef COWON_D2
    /* Set DAI clock divided from PLL0 (192MHz).
       The best approximation of 256*44.1kHz is 11.291MHz. */
    BCLKCTR &= ~DEV_DAI;
    PCLK_DAI = (1<<28) | 61682;  /* DCO mode */
    BCLKCTR |= DEV_DAI;

    /* Enable DAI block in Master mode, 256fs->32fs, 16bit LSB */
    DAMR = 0x3c8e80;
#elif defined(IAUDIO_7)
    BCLKCTR &= ~DEV_DAI;
    PCLK_DAI = (0x800a << 16) | (PCLK_DAI & 0xffff);
    BCLKCTR |= DEV_DAI;

    /* Master mode, 256->64fs, 16bit LSB*/
    DAMR = 0x3cce20;
#elif defined(LOGIK_DAX)
    /* TODO */
#elif defined(SANSA_M200)
    /* TODO */
#elif defined(SANSA_C100)
    /* TODO */
#else
#error "Target isn't supported"
#endif
    /* Set DAI interrupts as FIQs */
    IRQSEL = ~(DAI_RX_IRQ_MASK | DAI_TX_IRQ_MASK);

    /* Initialize default register values. */
    audiohw_init();

    dma_play_data.size = 0;
#if NUM_CORES > 1
    dma_play_data.core = 0; /* no core in control */
#endif
}

void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    (void)settings;
}

static void play_start_pcm(bool start)
{
    if (start)
    {
        DAMR &= ~(1<<14);   /* disable tx */
    }

    if (dma_play_data.frames < 4)
    {
        dma_play_data.frames = 0;
        return;
    }

    DADO_L(0) = *dma_play_data.p++;
    DADO_R(0) = *dma_play_data.p++;
    DADO_L(1) = *dma_play_data.p++;
    DADO_R(1) = *dma_play_data.p++;
    DADO_L(2) = *dma_play_data.p++;
    DADO_R(2) = *dma_play_data.p++;
    DADO_L(3) = *dma_play_data.p++;
    DADO_R(3) = *dma_play_data.p++;

    dma_play_data.frames -= 4;

    if (start)
    {
        DAMR |= (1<<14);   /* enable tx */
        dma_play_data.state = DAI_TX_IRQ_MASK;
    }
}

static void play_stop_pcm(bool reset)
{
    DAMR &= ~(1<<14);   /* disable tx */
    dma_play_data.state = 0;

    if (reset)
    {
        dma_play_data.addr = NULL;
        dma_play_data.frames = 0;
    }
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    dma_play_data.p_r = addr;
    dma_play_data.frames = frames;
    play_start_pcm(!dma_play_data.state);
}

void pcm_play_dma_prepare(void)
{
    pcm_play_dma_stop();
}

void pcm_play_dma_stop(void)
{
    play_stop_pcm(true);
}

void pcm_play_dma_lock(void)
{
    bitclr32(&IEN, DAI_TX_IRQ_MASK);
}

void pcm_play_dma_unlock(void)
{
    bitset32(&IEN, dma_play_data.state);
}

void pcm_play_dma_pause(bool pause)
{
    if (pause) {
        play_stop_pcm(false);
    } else {
        play_start_pcm(true);
    }
}

unsigned long pcm_get_frames_waiting(void)
{
    unsigned long frames = dma_play_data.frames;
    return frames < 4 ? 0 : frames;
}

#ifdef HAVE_RECORDING
/* TODO: implement */
void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    (void) addr;
    (void) frames;
}

void pcm_rec_dma_prepare(void)
{
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

const void * pcm_rec_dma_get_peak_buffer(unsigned long *frames_avail)
{
    *frames_avail = 0;
    return NULL;
}
#endif

#if defined(CPU_TCC77X) || defined(CPU_TCC780X)
void fiq_handler(void) ICODE_ATTR __attribute__((naked));
void fiq_handler(void)
{
    /* r10 contains DADO_L0 base address (set in crt0.S to minimise code in the
     * FIQ handler. r11 contains address of p (also set in crt0.S). Most other
     * addresses we need are generated by using offsets with these two.
     * r8 and r9 contains local copies of p and size respectively.
     * r0-r3 and r12 is a working register.
     */
    asm volatile (
#if defined(CPU_TCC780X)
        "mov     r8, #0xc000         \n" /* DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK */
        "mov     r9, #0xf3000000     \n" /* CREQ = 0xf3001004 */
        "orr     r9, r9, #0x00000f00 \n"
#elif defined(CPU_TCC77X)
        "mov     r8, #0x0030         \n" /* DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK */
        "mov     r9, #0x80000000     \n" /* CREQ = 0x80000104 */
#endif
        "str     r8, [r9, #0x104]    \n" /* clear DAI IRQs */
        "ldmia   r11, { r8-r9 }      \n" /* r8 = p, r9 = frames */
        "subs    r9, r9, #4          \n" /* >= 4 frames? */
        "blo     1f                  \n"
        "ldr     r12, [r8], #4       \n" /* load two samples */
        "str     r12, [r10, #0x0]    \n" /* write top sample to DADO_L0 */
        "mov     r12, r12, lsr #16   \n" /* put right sample at the bottom */
        "str     r12, [r10, #0x4]    \n" /* write low sample to DADO_R0*/
        "ldr     r12, [r8], #4       \n" /* load two samples */
        "str     r12, [r10, #0x8]    \n" /* write top sample to DADO_L1 */
        "mov     r12, r12, lsr #16   \n" /* put right sample at the bottom */
        "str     r12, [r10, #0xc]    \n" /* write low sample to DADO_R1*/
        "ldr     r12, [r8], #4       \n" /* load two samples */
        "str     r12, [r10, #0x10]   \n" /* write top sample to DADO_L2 */
        "mov     r12, r12, lsr #16   \n" /* put right sample at the bottom */
        "str     r12, [r10, #0x14]   \n" /* write low sample to DADO_R2*/
        "ldr     r12, [r8], #4       \n" /* load two samples */
        "str     r12, [r10, #0x18]   \n" /* write top sample to DADO_L3 */
        "mov     r12, r12, lsr #16   \n" /* put right sample at the bottom */
        "str     r12, [r10, #0x1c]   \n" /* write low sample to DADO_R3*/
        "stmia   r11, { r8-r9 }      \n" /* save p and frames */
        "subs    pc, lr, #4          \n" /* -> exit */
    "1:                              \n"
        "movlo   r9, #0              \n" /* zero-out size if needed */
        "strlo   r9, [r11, #4]       \n"
        "sub     lr, lr, #4          \n" /* stack scratch regs and lr */
        "stmfd   sp!, { r0-r3, lr }  \n"
        "ldr     r1, .complete       \n" /* pcm_play_dma_complete_callback(0) */
        "mov     r0, #0              \n"
        "blx     r1                  \n"
        "ldmfd   sp!, { r0-r3, pc }^ \n"
    ".complete:                      \n"
        ".word pcm_play_dma_complete_callback \n"
    );
}
#else /* C version for reference */
void fiq_handler(void) ICODE_ATTR;
void fiq_handler(void)
{
    /* Clear FIQ status */
    CREQ = DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK;

    if (dma_play_data.frames >= 4)
    {
        dma_play_data.frames -= 4;

        DADO_L(0) = *dma_play_data.p++;
        DADO_R(0) = *dma_play_data.p++;
        DADO_L(1) = *dma_play_data.p++;
        DADO_R(1) = *dma_play_data.p++;
        DADO_L(2) = *dma_play_data.p++;
        DADO_R(2) = *dma_play_data.p++;
        DADO_L(3) = *dma_play_data.p++;
        DADO_R(3) = *dma_play_data.p++;
    }
    else
    {
        dma_play_data.frames = 0;
        pcm_play_dma_complete_callback(0);
    }
}
#endif

/* TODO: required by wm8731 codec */
void i2s_reset(void)
{
    /* DAMR = 0; */
}
