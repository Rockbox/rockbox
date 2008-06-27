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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
    /*audiohw_postinit();*/
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
    /* Set DAI clock divided from PLL0 (192MHz).
       The best approximation of 256*44.1kHz is 11.291MHz. */
    BCLKCTR &= ~DEV_DAI;
    PCLK_DAI = (1<<28) | 61682;  /* DCO mode */
    BCLKCTR |= DEV_DAI;
    
    /* Enable DAI block in Master mode, 256fs->32fs, 16bit LSB */
    DAMR = 0x3c8e80;
    DAVC = 0x0;         /* Digital Volume = max */
    
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
    (void) pause;
}

size_t pcm_get_bytes_waiting(void)
{
    return dma_play_data.size & ~3;
}

#if 1
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
        "mov     r8, #0xc000         \n" /* DAI_TX_IRQ_MASK | DAI_RX_IRQ_MASK */
        "ldr     r9, =0xf3001004     \n" /* CREQ */
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
