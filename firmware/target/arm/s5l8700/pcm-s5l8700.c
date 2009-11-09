/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2009 Bertrik Sikken
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
#include <string.h>

#include "config.h"
#include "system.h"
#include "audio.h"
#include "s5l8700.h"
#include "panic.h"
#include "audiohw.h"
#include "pcm.h"
#include "pcm_sampr.h"
#include "mmu-target.h"

/*  Driver for the IIS/PCM part of the s5l8700 using DMA

    Notes:
    - not all possible PCM sample rates are enabled (no support in codec driver)
    - pcm_play_dma_pause is untested, not sure if implemented the right way
    - pcm_play_dma_stop is untested, not sure if implemented the right way
    - pcm_play_dma_get_peak_buffer is not implemented
    - recording is not implemented
*/

static volatile int locked = 0;
size_t nextsize;
size_t dblbufsize;
int dmamode;
const unsigned char* dblbuf;

/* table of recommended PLL/MCLK dividers for mode 256Fs from the datasheet */
static const struct div_entry {
    int             pdiv, mdiv, sdiv, cdiv;
} div_table[HW_NUM_FREQ] = {
#ifdef IPOD_NANO2G
    [HW_FREQ_11] = {   2,   41,    5,    4},
    [HW_FREQ_22] = {   2,   41,    4,    4},
    [HW_FREQ_44] = {   2,   41,    3,    4},
    [HW_FREQ_88] = {   2,   41,    2,    4},
#if 0   /* disabled because the codec driver does not support it (yet) */
    [HW_FREQ_8 ] = {   2,   12,    3,    9},
    [HW_FREQ_16] = {   2,   12,    2,    9},
    [HW_FREQ_32] = {   2,   12,    1,    9},
    [HW_FREQ_12] = {   2,   12,    4,    3},
    [HW_FREQ_24] = {   2,   12,    3,    3},
    [HW_FREQ_48] = {   2,   12,    2,    3},
    [HW_FREQ_96] = {   2,   12,    1,    3},
#endif
#else
    [HW_FREQ_11] = {  26,  189,    3,    8},
    [HW_FREQ_22] = {  50,   98,    2,    8},
    [HW_FREQ_44] = {  37,  151,    1,    9},
    [HW_FREQ_88] = {  50,   98,    1,    4},
#if 0   /* disabled because the codec driver does not support it (yet) */
    [HW_FREQ_8 ] = {  28,  192,    3,   12},
    [HW_FREQ_16] = {  28,  192,    3,    6},
    [HW_FREQ_32] = {  28,  192,    2,    6},
    [HW_FREQ_12] = {  28,  192,    3,    8},
    [HW_FREQ_24] = {  28,  192,    2,    8},
    [HW_FREQ_48] = {  28,  192,    2,    4},
    [HW_FREQ_96] = {  28,  192,    1,    4},
#endif
#endif
};

/* Mask the DMA interrupt */
void pcm_play_lock(void)
{
    if (locked++ == 0) {
        INTMSK &= ~(1 << 10);
    }
}

/* Unmask the DMA interrupt if enabled */
void pcm_play_unlock(void)
{
    if (--locked == 0) {
        INTMSK |= (1 << 10);
    }
}

static const void* dma_callback(void) ICODE_ATTR __attribute__((unused));
static const void* dma_callback(void)
{
    if (dmamode)
    {
        unsigned char *dma_start_addr;
        register pcm_more_callback_type get_more = pcm_callback_for_more;
        if (get_more)
        {
            get_more(&dma_start_addr, &nextsize);
            if (nextsize >= 4096)
            {
                dblbufsize = nextsize >> 4;
                nextsize = nextsize - dblbufsize;
                dblbuf = dma_start_addr + nextsize;
                dmamode = 0;
            }
            nextsize = (nextsize >> 1) - 1;
            clean_dcache();
            return dma_start_addr;
        }
        else
        {
            nextsize = -1;
            return 0;
        }
    }
    else
    {
        dmamode = 1;
        nextsize = (dblbufsize >> 1) - 1;
        return dblbuf;
    }
}

void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked)) ICODE_ATTR;
void fiq_handler(void)
{
    asm volatile (
        "cmn    r11, #1                            \n"
        "strne  r10, [r8]                          \n"  /* DMABASE0 */
        "strne  r11, [r8,#0x08]                    \n"  /* DMATCNT0 */
        "strne  r9, [r8,#0x14]                     \n"  /* DMACOM0 */
        "moveq  r10, #5                            \n"  /* STOP DMA */
        "streq  r10, [r8,#0x14]                    \n"  /* DMACOM0 */
        "mov    r10, #7                            \n"  /* CLEAR IRQ */
        "str    r10, [r8,#0x14]                    \n"  /* DMACOM0 */
        "mov    r11, #0x39C00000                   \n"  /* SRCPND */
        "mov    r10, #0x00000400                   \n"  /* INT_DMA */
        "str    r10, [r11]                         \n"  /* ACK FIQ */
        "stmfd  sp!, {r0-r3,lr}                    \n"
        "ldreq  r0, =pcm_play_dma_stopped_callback \n"
        "ldrne  r0, =dma_callback                  \n"
        "mov    lr, pc                             \n"
        "bx     r0                                 \n"
        "mov    r10, r0                            \n"
        "ldmfd  sp!, {r0-r3,lr}                    \n"
        "ldr    r11, =nextsize                     \n"
        "ldr    r11, [r11]                         \n"
        "subs   pc, lr, #4                         \n"
    );
}

void bootstrap_fiq(const void* addr, size_t tcnt) __attribute__((naked,noinline));
void bootstrap_fiq(const void* addr, size_t tcnt)
{
    (void)addr;
    (void)tcnt;
    asm volatile (
        "add    r2, lr, #4        \n"
        "mrs    r3, cpsr          \n"
        "msr    cpsr_c, #0xD1     \n"  /* FIQ mode, IRQ/FIQ disabled */
        "mov    r8, #0x38400000   \n"  /* DMA BASE */
        "mov    r9, #4            \n"  /* START DMA */
        "mov    r10, r0           \n"
        "mov    r11, r1           \n"
        "mov    r0, #0            \n"
        "ldr    r12, =fiq_handler \n"
        "ldr    sp, =_fiqstackend \n"
        "mov    lr, r2            \n"
        "msr    spsr_all, r3      \n"
        "bx     r12               \n"
    );
}

void pcm_play_dma_start(const void *addr_in, size_t size)
{
    unsigned char* addr = (unsigned char*)addr_in;

    /* S1: DMA channel 0 set */
    DMACON0 = (0 << 30) |       /* DEVSEL */
              (1 << 29) |       /* DIR */
              (0 << 24) |       /* SCHCNT */
              (1 << 22) |       /* DSIZE */
              (0 << 19) |       /* BLEN */
              (0 << 18) |       /* RELOAD */
              (0 << 17) |       /* HCOMINT */
              (1 << 16) |       /* WCOMINT */
              (0 << 0);         /* OFFSET */

#ifdef IPOD_NANO2G
    PCON5 = (PCON5 & ~(0xFFFF0000)) | 0x77720000;
    PCON6 = (PCON6 & ~(0x0F000000)) | 0x02000000;

    I2STXCON = (1 << 20) |  /* undocumented */
               (0 << 16) |  /* burst length */
               (0 << 15) |  /* 0 = falling edge */
               (0 << 13) |  /* 0 = basic I2S format */
               (0 << 12) |  /* 0 = MSB first */
               (0 << 11) |  /* 0 = left channel for low polarity */
               (5 << 8) |   /* MCLK divider */
               (0 << 5) |   /* 0 = 16-bit */
               (2 << 3) |   /* bit clock per frame */
               (1 << 0);    /* channel index */
#else
    /* S2: IIS Tx mode set */
    I2STXCON = (DMA_IISOUT_BLEN << 16) |  /* burst length */
               (0 << 15) |  /* 0 = falling edge */
               (0 << 13) |  /* 0 = basic I2S format */
               (0 << 12) |  /* 0 = MSB first */
               (0 << 11) |  /* 0 = left channel for low polarity */
               (3 << 8) |   /* MCLK divider */
               (0 << 5) |   /* 0 = 16-bit */
               (0 << 3) |   /* bit clock per frame */
               (1 << 0);    /* channel index */
#endif
    
    /* S3: DMA channel 0 on */
    if (!size)
    {
        register pcm_more_callback_type get_more = pcm_callback_for_more;
        if (get_more) get_more(&addr, &size);
        else return;  /* Nothing to play!? */
    }
    if (!size) return;  /* Nothing to play!? */
    clean_dcache();
    if (size >= 4096)
    {
        dblbufsize = size >> 4;
        size = size - dblbufsize;
        dblbuf = addr + size;
        dmamode = 0;
    }
    else dmamode = 1;
    bootstrap_fiq(addr, (size >> 1) - 1);

    /* S4: IIS Tx clock on */
    I2SCLKCON = (1 << 0);   /* 1 = power on */
    
    /* S5: IIS Tx on */
    I2STXCOM = (1 << 3) |   /* 1 = transmit mode on */
               (1 << 2) |   /* 1 = I2S interface enable */
               (1 << 1) |   /* 1 = DMA request enable */
               (0 << 0);    /* 0 = LRCK on */
}

void pcm_play_dma_stop(void)
{
    /* DMA channel off */
    DMACOM0 = 5;
    
    /* TODO Some time wait */
    /* LRCK half cycle wait */

    /* IIS Tx off */
    I2STXCOM = (1 << 3) |   /* 1 = transmit mode on */
               (0 << 2) |   /* 1 = I2S interface enable */
               (1 << 1) |   /* 1 = DMA request enable */
               (0 << 0);    /* 0 = LRCK on */
}

/* pause playback by disabling the I2S interface */
void pcm_play_dma_pause(bool pause)
{
    if (pause) {
        I2STXCOM |= (1 << 0);   /* LRCK off */
    }
    else {
        I2STXCOM &= ~(1 << 0);  /* LRCK on */
    }
}

void pcm_play_dma_init(void)
{
    /* configure IIS pins */
#ifdef IPOD_NANO2G
    PCON5 = (PCON5 & ~(0xFFFF0000)) | 0x22220000;
    PCON6 = (PCON6 & ~(0x0F000000)) | 0x02000000;
#else
    PCON7 = (PCON7 & ~(0x0FFFFF00)) | 0x02222200;
#endif

    /* enable clock to the IIS module */
    PWRCON &= ~(1 << 6);

    /* Enable the DMA FIQ */
    INTMOD |= (1 << 10);
    INTMSK |= (1 << 10);
    
    audiohw_preinit();
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

/* set the configured PCM frequency */
void pcm_dma_apply_settings(void)
{
  //    audiohw_set_frequency(pcm_sampr);
    
    struct div_entry div = div_table[pcm_fsel];

    PLLCON &= ~4;
    PLLCON &= ~0x10;
    PLLCON &= 0x3f;
    PLLCON |= 4;

    /* configure PLL1 and MCLK for the desired sample rate */
    PLL1PMS = (div.pdiv << 16) |
              (div.mdiv << 8) |
              (div.sdiv << 0);
    PLL1LCNT = 7500;    /* no idea what to put here */

    /* enable PLL1 and wait for lock */
    PLLCON |= (1 << 1);
    while ((PLLLOCK & (1 << 1)) == 0);

    /* configure MCLK */
    CLKCON = (CLKCON & ~(0xFF)) | 
             (0 << 7) |         /* MCLK_MASK */
             (2 << 5) |         /* MCLK_SEL = PLL1 */
             (1 << 4) |         /* MCLK_DIV_ON */
             (div.cdiv - 1);    /* MCLK_DIV_VAL */
}

size_t pcm_get_bytes_waiting(void)
{
    return (nextsize + DMACTCNT0 + 2) << 1;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    *count = DMACTCNT0 >> 1;
    return (void *)(((DMACADDR0 + 2) & ~3) | 0x40000000);
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL)
        addr = (void*)((uintptr_t)addr | 0x40000000);
    return addr;
}
#endif


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

void pcm_record_more(void *start, size_t size)
{
    (void)start;
    (void)size;
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_rec_dma_close(void)
{
}


void pcm_rec_dma_init(void)
{
}


const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    (void)count;
}

#endif /* HAVE_RECORDING */
