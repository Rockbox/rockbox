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
#include "pcm.h"
#include "pcm-internal.h"

struct dma_data
{
/* NOTE: The order of size and p is important if you use assembler
   optimised fiq handler, so don't change it. */
    union
    {
        uint16_t *p;
        const void *p_r;
        void *p_w;
    };
    size_t size;
#if NUM_CORES > 1
    unsigned core;
#endif
    int locked;
    int state;
};

/****************************************************************************
 ** Playback DMA transfer
 **/
struct dma_data dma_play_data SHAREDBSS_ATTR =
{
    /* Initialize to a locked, stopped state */
    { .p = NULL },
    .size = 0,
#if NUM_CORES > 1
    .core = 0x00,
#endif
    .locked = 0,
    .state = 0
};

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    unsigned long addr = (unsigned long)dma_play_data.p;
    size_t cnt = dma_play_data.size;
    *count = cnt >> 2;
    return (void *)((addr + 2) & ~3);
}

void pcm_play_dma_init(void)
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

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

void pcm_dma_apply_settings(void)
{
}

static void play_start_pcm(void)
{
    DAMR &= ~(1<<14);   /* disable tx */
    dma_play_data.state = 1;

    if (dma_play_data.size >= 16)
    {
        DADO_L(0) = *dma_play_data.p++;
        DADO_R(0) = *dma_play_data.p++;
        DADO_L(1) = *dma_play_data.p++;
        DADO_R(1) = *dma_play_data.p++;
        DADO_L(2) = *dma_play_data.p++;
        DADO_R(2) = *dma_play_data.p++;
        DADO_L(3) = *dma_play_data.p++;
        DADO_R(3) = *dma_play_data.p++;
        dma_play_data.size -= 16;
    }

    DAMR |= (1<<14);   /* enable tx */
}

static void play_stop_pcm(void)
{
    DAMR &= ~(1<<14);   /* disable tx */
    dma_play_data.state = 0;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    dma_play_data.p_r  = addr;
    dma_play_data.size = size;

#if NUM_CORES > 1
    /* This will become more important later - and different ! */
    dma_play_data.core = processor_id(); /* save initiating core */
#endif

    IEN |= DAI_TX_IRQ_MASK;

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    play_stop_pcm();
    dma_play_data.size = 0;
#if NUM_CORES > 1
    dma_play_data.core = 0; /* no core in control */
#endif
}

void pcm_play_lock(void)
{
    int status = disable_fiq_save();

    if (++dma_play_data.locked == 1)
    {
        IEN &= ~DAI_TX_IRQ_MASK;
    }

    restore_fiq(status);
}

void pcm_play_unlock(void)
{
   int status = disable_fiq_save();

    if (--dma_play_data.locked == 0 && dma_play_data.state != 0)
    {
        IEN |= DAI_TX_IRQ_MASK;
    }

   restore_fiq(status);
}

void pcm_play_dma_pause(bool pause)
{
    if (pause) {
        play_stop_pcm();
    } else {
        play_start_pcm();
    }
}

size_t pcm_get_bytes_waiting(void)
{
    return dma_play_data.size & ~3;
}

#ifdef HAVE_RECORDING
/* TODO: implement */
void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void) addr;
    (void) size;
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

const void * pcm_rec_dma_get_peak_buffer(void)
{
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
        "sub     lr, lr, #4          \n"
        "stmfd   sp!, { r0-r3, lr }  \n" /* stack scratch regs and lr */
        "mov     r14, #0             \n" /* Was the callback called? */
#if defined(CPU_TCC780X)
        "mov     r8, #0xc000         \n" /* DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK */
        "ldr     r9, =0xf3001004     \n" /* CREQ */
#elif defined(CPU_TCC77X)
        "mov     r8, #0x0030         \n" /* DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK */
        "ldr     r9, =0x80000104     \n" /* CREQ */
#endif
        "str     r8, [r9]            \n" /* clear DAI IRQs */
        "ldmia   r11, { r8-r9 }      \n" /* r8 = p, r9 = size */
        "cmp     r9, #0x10           \n" /* is size <16? */
        "blo     .more_data          \n" /* if so, ask pcmbuf for more data */

    ".fill_fifo:                     \n"
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
        "sub     r9, r9, #0x10       \n" /* 4 words written */
        "stmia   r11, { r8-r9 }      \n" /* save p and size */

        "cmp     r14, #0             \n" /* Callback called? */
        "ldmeqfd sp!, { r0-r3, pc }^ \n" /* no? -> exit */

        "ldr     r1, =pcm_play_status_callback \n"
        "ldr     r1, [r1]            \n"
        "cmp     r1, #0              \n"
        "movne   r0, %1              \n"
        "blxne   r1                  \n"
        "ldmfd   sp!, { r0-r3, pc }^ \n" /* exit */

    ".more_data:                     \n"
        "mov     r14, #1             \n" /* Remember we got more data in this FIQ */
        "mov     r0, %0              \n" /* r0 = status */
        "mov     r1, r11             \n" /* r1 = &dma_play_data.p_r */
        "add     r2, r11, #4         \n" /* r2 = &dma_play_data.size */
        "mov     lr, pc              \n"
        "ldr     pc, =pcm_play_dma_complete_callback \n"
        "cmp     r0, #0              \n" /* any more to play? */
        "ldmneia r11, { r8-r9 }      \n" /* load new p and size */
        "cmpne   r9, #0x0f           \n" /* did we actually get enough data? */
        "bhi     .fill_fifo          \n" /* not stop and enough? refill */
        "ldmfd   sp!, { r0-r3, pc }^ \n" /* exit */
        ".ltorg                      \n"
        : : "i"(PCM_DMAST_OK), "i"(PCM_DMAST_STARTED)
    );
}
#else /* C version for reference */
void fiq_handler(void) ICODE_ATTR;
void fiq_handler(void)
{
    register bool new_buffer = false;

    if (dma_play_data.size < 16)
    {
        /* p is empty, get some more data */
        new_buffer = pcm_play_dma_complete_callback(&dma_play_data.p_r,
                                                    &dma_play_data.size);
    }

    if (dma_play_data.size >= 16)
    {
        DADO_L(0) = *dma_play_data.p++;
        DADO_R(0) = *dma_play_data.p++;
        DADO_L(1) = *dma_play_data.p++;
        DADO_R(1) = *dma_play_data.p++;
        DADO_L(2) = *dma_play_data.p++;
        DADO_R(2) = *dma_play_data.p++;
        DADO_L(3) = *dma_play_data.p++;
        DADO_R(3) = *dma_play_data.p++;

        dma_play_data.size -= 16;
    }

    /* Clear FIQ status */
    CREQ = DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK;

    if (new_buffer)
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
}
#endif

/* TODO: required by wm8731 codec */
void i2s_reset(void)
{
    /* DAMR = 0; */
}
