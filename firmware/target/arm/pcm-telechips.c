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
#include "pcm.h"

/* Just for tests enable it to play simple tone */
//#define PCM_TELECHIPS_TEST

struct dma_data
{
/* NOTE: The order of size and p is important if you use assembler
   optimised fiq handler, so don't change it. */
    uint16_t *p;
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
    .p = NULL,
    .size = 0,
#if NUM_CORES > 1
    .core = 0x00,
#endif
    .locked = 0,
    .state = 0
};

static unsigned long pcm_freq SHAREDDATA_ATTR = HW_SAMPR_DEFAULT; /* 44.1 is default */

void pcm_postinit(void)
{
#if defined(IAUDIO_7)
    audiohw_postinit(); /* implemented not for all codecs */
#endif
    pcm_apply_settings();
}

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
    PCLK_DAI = (0x800b << 16) | (PCLK_DAI & 0xffff);
    BCLKCTR |= DEV_DAI;
    /* Master mode, 256->64fs, 16bit LSB*/
    DAMR = 0x3cce20;
#elif defined(LOGIK_DAX)
    /* TODO */
#elif defined(SANSA_M200)
    /* TODO */
#else
#error "Target isn't supported"
#endif
    /* Set DAI interrupts as FIQs */
    IRQSEL = ~(DAI_RX_IRQ_MASK | DAI_TX_IRQ_MASK);
    
    pcm_set_frequency(SAMPR_44);

    /* Initialize default register values. */
    audiohw_init();

    /* Power on */
    audiohw_enable_output(true);
    
    /* Unmute the master channel (DAC should be at zero point now). */
    audiohw_mute(false);

    dma_play_data.size = 0;
#if NUM_CORES > 1
    dma_play_data.core = 0; /* no core in control */
#endif
}

void pcm_apply_settings(void)
{
    pcm_curr_sampr = pcm_freq;
}

void pcm_set_frequency(unsigned int frequency)
{
    (void) frequency;
    pcm_freq = HW_SAMPR_DEFAULT;
}

static void play_start_pcm(void)
{
    pcm_apply_settings();

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
    dma_play_data.p    = (void *)(((uintptr_t)addr + 2) & ~3);
    dma_play_data.size = (size & ~3);

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

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    *count = 0;
    return NULL;
}

void pcm_record_more(void *start, size_t size)
{
    (void) start;
    (void) size;
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
        "ldr     r9, =0xf3001004     \n" /* CREQ */
#elif defined(CPU_TCC77X)
        "mov     r8, #0x0030         \n" /* DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK */
        "ldr     r9, =0x80000104     \n" /* CREQ */
#endif
        "str     r8, [r9]            \n" /* clear DAI IRQs */
        "ldmia   r11, { r8-r9 }      \n" /* r8 = p, r9 = size */
        "cmp     r9, #0x10           \n" /* is size <16? */
        "blt     .more_data          \n" /* if so, ask pcmbuf for more data */

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

    ".exit:                          \n"
        "subs    pc, lr, #4          \n" /* FIQ specific return sequence */

    ".more_data:                     \n"
        "stmfd   sp!, { r0-r3, lr }  \n" /* stack scratch regs and lr */
        "ldr     r2, =pcm_callback_for_more \n"
        "ldr     r2, [r2]            \n" /* get callback address */
        "cmp     r2, #0              \n" /* check for null pointer */
        "movne   r0, r11             \n" /* r0 = &p */
        "addne   r1, r11, #4         \n" /* r1 = &size */
        "blxne   r2                  \n" /* call pcm_callback_for_more */
        "ldmia   r11, { r8-r9 }      \n" /* reload p and size */
        "cmp     r9, #0x10           \n" /* did we actually get more data? */
        "ldmgefd sp!, { r0-r3, lr }  \n"
        "bge     .fill_fifo          \n" /* yes: fill the fifo */
        "ldr     r12, =pcm_play_dma_stop \n"
        "blx     r12                 \n" /* no: stop playback */
        "ldr     r12, =pcm_play_dma_stopped_callback \n"
        "blx     r12                 \n"
        "ldmfd   sp!, { r0-r3, lr }  \n"
        "b       .exit               \n"
        ".ltorg                      \n"
    );
}
#else /* C version for reference */
void fiq_handler(void) ICODE_ATTR __attribute__((naked));
void fiq_handler(void)
{
    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */
                    
    register pcm_more_callback_type get_more;

    if (dma_play_data.size < 16)
    {
        /* p is empty, get some more data */
        get_more = pcm_callback_for_more;
        if (get_more)
        {
            get_more((unsigned char**)&dma_play_data.p,
                     &dma_play_data.size);
        }
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
    else
    {
        /* No more data, so disable the FIFO/interrupt */
        pcm_play_dma_stop();
        pcm_play_dma_stopped_callback();
    }
    
    /* Clear FIQ status */
    CREQ = DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK;
    
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from FIQ */
}
#endif

/* TODO: required by wm8531 codec, why not to implement  */
void i2s_reset(void)
{
/* DAMR = 0; */
}

#ifdef PCM_TELECHIPS_TEST
#include "lcd.h"
#include "sprintf.h"
#include "backlight-target.h"

static int frame = 0;
static void test_callback_for_more(unsigned char **start, size_t *size)
{
    static unsigned short data[8];
    static int cntr = 0;
    int i;

    for (i = 0; i < 8; i ++) {
        unsigned short val;

        if (0x100 == (cntr & 0x100))
            val = 0x0fff;
        else
            val = 0x0000;
        data[i] = val;
        cntr++;
    }

    *start = data;
    *size = sizeof(data);

    frame++;
}

void pcm_telechips_test(void)
{
    static char buf[100];
    unsigned char *data;
    size_t size;
    
    _backlight_on();

    audiohw_preinit();
    pcm_play_dma_init();
    pcm_postinit();

    audiohw_mute(false);
    audiohw_set_master_vol(0x7f, 0x7f);

    pcm_callback_for_more = test_callback_for_more;    
    test_callback_for_more(&data, &size);
    pcm_play_dma_start(data, size);

    while (1) {
        int line = 0;
        lcd_clear_display();
        lcd_puts(0, line++, __func__);
        snprintf(buf, sizeof(buf), "frame: %d", frame);
        lcd_puts(0, line++, buf);
        lcd_update();
        sleep(1);
    }
}
#endif
