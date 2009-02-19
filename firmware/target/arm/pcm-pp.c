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
#include "pcm_sampr.h"

/** DMA **/

#ifdef CPU_PP502x
/* 16-bit, L-R packed into 32 bits with left in the least significant halfword */
#define SAMPLE_SIZE   16
/* DMA Requests from IIS, Memory to peripheral, single transfer,
   wait for DMA request, interrupt on complete */
#define DMA_PLAY_CONFIG ((DMA_REQ_IIS << DMA_CMD_REQ_ID_POS) | \
                          DMA_CMD_RAM_TO_PER | DMA_CMD_SINGLE | \
                          DMA_CMD_WAIT_REQ | DMA_CMD_INTR)
/* DMA status cannot be viewed from outside code in control because that can
 * clear the interrupt from outside the handler and prevent the handler from
 * from being called. Split up transfers to a reasonable size that is good as
 * a timer, obtaining a keyclick position and peaking yet still keeps the
 * FIQ count low.
 */
#define MAX_DMA_CHUNK_SIZE (pcm_curr_sampr >> 6) /* ~1/256 seconds */
#else
/* 32-bit, one left 32-bit sample followed by one right 32-bit sample */
#define SAMPLE_SIZE   32
#endif

struct dma_data
{
/* NOTE: The order of size and p is important if you use assembler
   optimised fiq handler, so don't change it. */
    union
    {
        unsigned long addr;
        uint32_t *p16;  /* For packed 16-16 stereo pairs */
        uint16_t *p32;  /* For individual samples converted to 32-bit */
    };
    size_t size;
#if NUM_CORES > 1
    unsigned core;
#endif
    int locked;
    int state;
};

extern void *fiq_function;

/* Dispatch to the proper handler and leave the main vector table alone */
void fiq_handler(void) ICODE_ATTR __attribute__((naked));
void fiq_handler(void)
{
    asm volatile (
        "ldr pc, [pc, #-4] \n"
    "fiq_function:         \n"
        ".word 0           \n"
    );
}

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr)
{
    if (addr != NULL && (unsigned long)addr < UNCACHED_BASE_ADDR)
        addr = UNCACHED_ADDR(addr);
    return addr;
}
#endif

/* TODO: Get simultaneous recording and playback to work. Just needs some tweaking */

/****************************************************************************
 ** Playback DMA transfer
 **/
static struct dma_data dma_play_data IBSS_ATTR =
{
    /* Initialize to a locked, stopped state */
    { .addr = 0 },
    .size = 0,
#if NUM_CORES > 1
    .core = 0x00,
#endif
    .locked = 0,
    .state = 0
};

void pcm_dma_apply_settings(void)
{
    audiohw_set_frequency(pcm_fsel);
}

#if defined(CPU_PP502x)
/* NOTE: direct stack use forbidden by GCC stack handling bug for FIQ */
void ICODE_ATTR __attribute__((interrupt("FIQ"))) fiq_playback(void)
{
    register pcm_more_callback_type get_more;
    register size_t size;

    DMA0_STATUS; /* Clear any pending interrupt */

    size = (DMA0_CMD & 0xffff) + 4; /* Get size of trasfer that caused this
                                       interrupt */
    dma_play_data.addr += size;
    dma_play_data.size -= size;

    while (1)
    {
        if (dma_play_data.size > 0) {
            size = MAX_DMA_CHUNK_SIZE;
            /* Not at least MAX_DMA_CHUNK_SIZE left or there would be less
             * than a FIFO's worth of data after this transfer? */
            if (size + 16*4 > dma_play_data.size)
                size = dma_play_data.size;

            /* Set the new DMA values and activate channel */
            DMA0_RAM_ADDR = dma_play_data.addr;
            DMA0_CMD = DMA_PLAY_CONFIG | (size - 4) | DMA_CMD_START;
            return;
        }

        /* Buffer empty.  Try to get more. */
        get_more = pcm_callback_for_more;
        if (get_more) {
            get_more((unsigned char **)&dma_play_data.addr, &dma_play_data.size);
            dma_play_data.addr = (dma_play_data.addr + 3) & ~3;  
            dma_play_data.size &= 0xfffc;
        }

        if (dma_play_data.size <= 0) {
            break;
        }

        if (dma_play_data.addr < UNCACHED_BASE_ADDR) {
            /* Flush any pending cache writes */
            dma_play_data.addr = UNCACHED_ADDR(dma_play_data.addr);
            cpucache_flush();
        }
    }

    /* Callback missing or no more DMA to do */
    pcm_play_dma_stop();
    pcm_play_dma_stopped_callback();
}
#else
/* ASM optimised FIQ handler. Checks for the minimum allowed loop cycles by
 * evalutation of free IISFIFO-slots against available source buffer words.
 * Through this it is possible to move the check for IIS_TX_FREE_COUNT outside
 * the loop and do some further optimization. Right after the loops (source
 * buffer -> IISFIFO) are done we need to check whether we have to exit FIQ
 * handler (this must be done, if all free FIFO slots were filled) or we will
 * have to get some new source data. Important information kept from former
 * ASM implementation (not used anymore): GCC fails to make use of the fact
 * that FIQ mode has registers r8-r14 banked, and so does not need to be saved.
 * This routine uses only these registers, and so will never touch the stack
 * unless it actually needs to do so when calling pcm_callback_for_more.
 * C version is still included below for reference and testing.
 */
#if 1
void fiq_playback(void) ICODE_ATTR __attribute__((naked));
void fiq_playback(void)
{
    /* r10 contains IISCONFIG address (set in crt0.S to minimise code in actual
     * FIQ handler. r11 contains address of p (also set in crt0.S). Most other
     * addresses we need are generated by using offsets with these two.
     * r10 + 0x40 is IISFIFO_WR, and r10 + 0x0c is IISFIFO_CFG.
     * r8 and r9 contains local copies of p and size respectively.
     * r0-r3 and r12 is a working register.
     */
    asm volatile (
        "stmfd   sp!, { r0-r3, lr }  \n" /* stack scratch regs and lr */

#if CONFIG_CPU == PP5002
        "ldr     r12, =0xcf001040    \n" /* Some magic from iPodLinux */
        "ldr     r12, [r12]          \n"
#endif
        "ldmia   r11, { r8-r9 }      \n" /* r8 = p, r9 = size */
        "cmp     r9, #0              \n" /* is size 0? */
        "beq     .more_data          \n" /* if so, ask pcmbuf for more data */

#if SAMPLE_SIZE == 16
    ".check_fifo:                    \n"   
        "ldr     r0, [r10, %[cfg]]   \n" /* read IISFIFO_CFG to check FIFO status */
        "and     r0, r0, %[mask]     \n" /* r0 = IIS_TX_FREE_COUNT << 16 (PP502x) */

        "mov     r1, r0, lsr #16     \n" /* number of free FIFO slots */
        "cmp     r1, r9, lsr #2      \n" /* number of words from source */
        "movgt   r1, r9, lsr #2      \n" /* r1 = amount of allowed loops */
        "sub     r9, r9, r1, lsl #2  \n" /* r1 words will be written in following loop */

        "subs    r1, r1, #2          \n"
    ".fifo_loop_2:                   \n"
        "ldmgeia r8!, {r2, r12}      \n" /* load four samples */
        "strge   r2 , [r10, %[wr]]   \n" /* write sample 0-1 to IISFIFO_WR */
        "strge   r12, [r10, %[wr]]   \n" /* write sample 2-3 to IISFIFO_WR */
        "subges  r1, r1, #2          \n" /* one more loop? */
        "bge     .fifo_loop_2        \n" /* yes, continue */

        "tst     r1, #1              \n" /* two samples (one word) left? */
        "ldrne   r12, [r8], #4       \n" /* load two samples */
        "strne   r12, [r10, %[wr]]   \n" /* write sample 0-1 to IISFIFO_WR */

        "cmp     r9, #0              \n" /* either FIFO is full or source buffer is empty */
        "bgt     .exit               \n" /* if source buffer is not empty, FIFO must be full */
#elif SAMPLE_SIZE == 32
    ".check_fifo:                    \n"
        "ldr     r0, [r10, %[cfg]]   \n" /* read IISFIFO_CFG to check FIFO status */
        "and     r0, r0, %[mask]     \n" /* r0 = IIS_TX_FREE_COUNT << 23 (PP5002) */

        "movs    r1, r0, lsr #24     \n" /* number of free pairs of FIFO slots */
        "beq     .exit               \n" /* no complete pair? -> exit */
        "cmp     r1, r9, lsr #2      \n" /* number of words from source */
        "movgt   r1, r9, lsr #2      \n" /* r1 = amount of allowed loops */
        "sub     r9, r9, r1, lsl #2  \n" /* r1 words will be written in following loop */

    ".fifo_loop:                     \n"
        "ldr     r12, [r8], #4       \n" /* load two samples */
        "mov     r2 , r12, lsl #16   \n" /* put left sample at the top bits */
        "str     r2 , [r10, %[wr]]   \n" /* write top sample to IISFIFO_WR */
        "str     r12, [r10, %[wr]]   \n" /* write low sample to IISFIFO_WR*/
        "subs    r1, r1, #1          \n" /* one more loop? */
        "bgt     .fifo_loop          \n" /* yes, continue */

        "cmp     r9, #0              \n" /* either FIFO is full or source buffer is empty */
        "bgt     .exit               \n" /* if source buffer is not empty, FIFO must be full */
#endif

    ".more_data:                     \n"
        "ldr     r2, =pcm_callback_for_more \n"
        "ldr     r2, [r2]            \n" /* get callback address */
        "cmp     r2, #0              \n" /* check for null pointer */
        "beq     .stop               \n" /* callback removed, stop */
        "stmia   r11, { r8-r9 }      \n" /* save internal copies of variables back */
        "mov     r0, r11             \n" /* r0 = &p */
        "add     r1, r11, #4         \n" /* r1 = &size */
        "mov     lr, pc              \n" /* call pcm_callback_for_more */
        "bx      r2                  \n"
        "ldmia   r11, { r8-r9 }      \n" /* reload p and size */
        "cmp     r9, #0              \n" /* did we actually get more data? */
        "bne     .check_fifo         \n"

    ".stop:                          \n" /* call termination routines */
        "ldr     r12, =pcm_play_dma_stop \n"
        "mov     lr, pc              \n"
        "bx      r12                 \n"
        "ldr     r12, =pcm_play_dma_stopped_callback \n"
        "mov     lr, pc              \n"
        "bx      r12                 \n"

    ".exit:                          \n" /* (r8=0 if stopping, look above) */
        "stmia   r11, { r8-r9 }      \n" /* save p and size */
        "ldmfd   sp!, { r0-r3, lr }  \n"
        "subs    pc, lr, #4          \n" /* FIQ specific return sequence */
        ".ltorg                      \n"
        : /* These must only be integers! No regs */
        : [mask]"i"(IIS_TX_FREE_MASK),
          [cfg]"i"((int)&IISFIFO_CFG - (int)&IISCONFIG),
          [wr]"i"((int)&IISFIFO_WR - (int)&IISCONFIG)
    );
}
#else /* C version for reference */
void fiq_playback(void) __attribute__((interrupt ("FIQ"))) ICODE_ATTR;
/* NOTE: direct stack use forbidden by GCC stack handling bug for FIQ */
void fiq_playback(void)
{
    register pcm_more_callback_type get_more;

#if CONFIG_CPU == PP5002
    inl(0xcf001040);
#endif

    do {
        while (dma_play_data.size > 0) {
            if (IIS_TX_FREE_COUNT < 2) {
                return;
            }
#if SAMPLE_SIZE == 16
            IISFIFO_WR = *dma_play_data.p16++;
#elif SAMPLE_SIZE == 32
            IISFIFO_WR = *dma_play_data.p32++ << 16;
            IISFIFO_WR = *dma_play_data.p32++ << 16;
#endif
            dma_play_data.size -= 4;
        }

        /* p is empty, get some more data */
        get_more = pcm_callback_for_more;
        if (get_more) {
            get_more((unsigned char**)&dma_play_data.addr,
                     &dma_play_data.size);
        }
    } while (dma_play_data.size);

    /* No more data, so disable the FIFO/interrupt */
    pcm_play_dma_stop();
    pcm_play_dma_stopped_callback();
}
#endif /* ASM / C selection */
#endif /* CPU_PP502x */

/* For the locks, FIQ must be disabled because the handler manipulates
   IISCONFIG and the operation is not atomic - dual core support
   will require other measures */
void pcm_play_lock(void)
{
    int status = disable_fiq_save();

    if (++dma_play_data.locked == 1) {
#ifdef CPU_PP502x
        CPU_INT_DIS = DMA0_MASK;
#else
        IIS_IRQTX_REG &= ~IIS_IRQTX;
#endif
    }

    restore_fiq(status);
}

void pcm_play_unlock(void)
{
   int status = disable_fiq_save();

    if (--dma_play_data.locked == 0 && dma_play_data.state != 0) {
#ifdef CPU_PP502x
        CPU_INT_EN = DMA0_MASK;
#else
        IIS_IRQTX_REG |= IIS_IRQTX;
#endif
    }

   restore_fiq(status);
}

static void play_start_pcm(void)
{
    fiq_function = fiq_playback;

#ifdef CPU_PP502x
    /* Not at least MAX_DMA_CHUNK_SIZE left or there would be less than a
     * FIFO's worth of data after this transfer? */
    size_t size = MAX_DMA_CHUNK_SIZE;
    if (dma_play_data.size + 16*4 < size)
        size = dma_play_data.size;

    DMA0_RAM_ADDR = dma_play_data.addr;
    DMA0_CMD = DMA_PLAY_CONFIG | (size - 4) | DMA_CMD_START;
    dma_play_data.state = 1;
#else
    IISCONFIG &= ~IIS_TXFIFOEN;  /* Stop transmitting */

    /* Fill the FIFO or start when data is used up */
    while (1) {
        if (IIS_TX_FREE_COUNT < 2 || dma_play_data.size == 0) {
            IISCONFIG |= IIS_TXFIFOEN; /* Start transmitting */
            dma_play_data.state = 1;
            return;
        }

#if SAMPLE_SIZE == 16
        IISFIFO_WR = *dma_play_data.p16++;
#elif SAMPLE_SIZE == 32
        IISFIFO_WR = *dma_play_data.p32++ << 16;
        IISFIFO_WR = *dma_play_data.p32++ << 16;
#endif
        dma_play_data.size -= 4;
    }
#endif
}

static void play_stop_pcm(void)
{
#ifdef CPU_PP502x
    size_t status = DMA0_STATUS;
    size_t size;

    /* Stop transfer */
    DMA0_CMD &= ~(DMA_CMD_START | DMA_CMD_INTR);

    /* Wait for not busy + clear int */
    while (DMA0_STATUS & (DMA_STATUS_BUSY | DMA_STATUS_INTR));

    size = (status & 0xfffc) + 4;

    if (status & DMA_STATUS_BUSY)
    {
        /* Transfer was interrupted - leave what's left */
        dma_play_data.addr += dma_play_data.size - size;
        dma_play_data.size = size;
    }
    else if (status & DMA_STATUS_INTR)
    {
        /* Tranfer was finished - DMA0_STATUS will have been reloaded
         * automatically with size in DMA0_CMD. */
        dma_play_data.addr += size;
        dma_play_data.size -= size;
        if (dma_play_data.size <= 0)
            dma_play_data.addr = 0; /* Entire buffer has completed */
    }
#else
    /* Disable TX interrupt */
    IIS_IRQTX_REG &= ~IIS_IRQTX;
#endif

    /* Wait for FIFO to empty */
    while (!IIS_TX_IS_EMPTY);

    dma_play_data.state = 0;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    addr = (void *)(((long)addr + 2) & ~3);
    size &= 0xfffc;

#if NUM_CORES > 1
    /* This will become more important later - and different ! */
    dma_play_data.core = processor_id(); /* save initiating core */
#endif

    pcm_play_dma_stop();

    if (size <= 0)
        return;

#ifdef CPU_PP502x
    if ((unsigned long)addr < UNCACHED_BASE_ADDR) {
        /* Flush any pending cache writes */
        addr = UNCACHED_ADDR(addr);
        cpucache_flush();
    }

    dma_play_data.addr = (unsigned long)addr;
    dma_play_data.size = size;
    DMA0_PER_ADDR = (unsigned long)&IISFIFO_WR;
    DMA0_FLAGS = DMA_FLAGS_UNK26;
    DMA0_INCR = DMA_INCR_RANGE_FIXED | DMA_INCR_WIDTH_32BIT;
#endif

    play_start_pcm();
}

/* Stops the DMA transfer and interrupt */
void pcm_play_dma_stop(void)
{
    play_stop_pcm();
    dma_play_data.addr = 0;
    dma_play_data.size = 0;
#if NUM_CORES > 1
    dma_play_data.core = 0; /* no core in control */
#endif
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

void pcm_play_dma_init(void)
{
    /* Initialize default register values. */
    audiohw_init();

#ifdef CPU_PP502x
    /* Enable DMA controller */
    DMA_MASTER_CONTROL |= DMA_MASTER_CONTROL_EN;
    /* FIQ priority for DMA0 */
    CPU_INT_PRIORITY |= DMA0_MASK;
    /* Enable request?? Not setting or clearing everything doesn't seem to
     * prevent it operating. Perhaps important for reliability (how requests
     * are handled). */
    DMA_REQ_STATUS |= 1ul << DMA_REQ_IIS;
    DMA0_STATUS;
#else
    /* Set up banked registers for FIQ mode */

    /* Use non-banked registers for scratch. */
    register volatile void *iiscfg asm("r0") = &IISCONFIG;
    register volatile void *dmapd asm("r1") = &dma_play_data;

    asm volatile (
        "mrs    r2, cpsr            \n" /* Save mode and interrupt status */
        "msr    cpsr_c, #0xd1       \n" /* Switch to FIQ mode */
        "mov    r8, #0              \n"
        "mov    r9, #0              \n"
        "mov    r10, %[iiscfg]      \n"
        "mov    r11, %[dmapd]       \n"
        "msr    cpsr_c, r2          \n"
        :
        : [iiscfg]"r"(iiscfg), [dmapd]"r"(dmapd)
        : "r2");

    /* FIQ priority for I2S */
    CPU_INT_PRIORITY |= IIS_MASK;
    CPU_INT_EN = IIS_MASK;
#endif

    IISCONFIG |= IIS_TXFIFOEN;
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    unsigned long addr, size;

    int status = disable_fiq_save();
    addr = dma_play_data.addr;
    size = dma_play_data.size;
    restore_fiq(status);

    *count = size >> 2;
    return (void *)((addr + 2) & ~3);
}

/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
/* PCM recording interrupt routine lockout */
static struct dma_data dma_rec_data IBSS_ATTR =
{
    /* Initialize to a locked, stopped state */
    { .addr = 0 },
    .size = 0,
#if NUM_CORES > 1
    .core = 0x00,
#endif
    .locked = 0,
    .state  = 0
};

/* For the locks, FIQ must be disabled because the handler manipulates
   IISCONFIG and the operation is not atomic - dual core support
   will require other measures */
void pcm_rec_lock(void)
{
    int status = disable_fiq_save();

    if (++dma_rec_data.locked == 1)
        IIS_IRQRX_REG &= ~IIS_IRQRX;

    restore_fiq(status);
}

void pcm_rec_unlock(void)
{
    int status = disable_fiq_save();

    if (--dma_rec_data.locked == 0 && dma_rec_data.state != 0)
        IIS_IRQRX_REG |= IIS_IRQRX;

    restore_fiq(status);
}

/* NOTE: direct stack use forbidden by GCC stack handling bug for FIQ */
void fiq_record(void) ICODE_ATTR __attribute__((interrupt ("FIQ")));

#if defined(SANSA_C200) || defined(SANSA_E200)
void fiq_record(void)
{
    register pcm_more_callback_type2 more_ready;
    register int32_t value;

    if (audio_channels == 2) {
        /* RX is stereo */
        while (dma_rec_data.size > 0) {
            if (IIS_RX_FULL_COUNT < 2) {
                return;
            }

            /* Discard every other sample since ADC clock is 1/2 LRCK */
            value = IISFIFO_RD;
            IISFIFO_RD;

            *dma_rec_data.p16++ = value;
            dma_rec_data.size -= 4;

            /* TODO: Figure out how to do IIS loopback */
            if (audio_output_source != AUDIO_SRC_PLAYBACK) {
                if (IIS_TX_FREE_COUNT >= 16) {
                    /* Resync the output FIFO - it ran dry */
                    IISFIFO_WR = 0;
                    IISFIFO_WR = 0;
                }
                IISFIFO_WR = value;
                IISFIFO_WR = value;
            }
        }
    }
    else {
        /* RX is left channel mono */
        while (dma_rec_data.size > 0) {
            if (IIS_RX_FULL_COUNT < 2) {
                return;
            }

            /* Discard every other sample since ADC clock is 1/2 LRCK */
            value = IISFIFO_RD;
            IISFIFO_RD;

            value = (uint16_t)value | (value << 16);

            *dma_rec_data.p16++ = value;
            dma_rec_data.size -= 4;

            if (audio_output_source != AUDIO_SRC_PLAYBACK) {
                if (IIS_TX_FREE_COUNT >= 16) {
                    /* Resync the output FIFO - it ran dry */
                    IISFIFO_WR = 0;
                    IISFIFO_WR = 0;
                }

                value = *((int32_t *)dma_rec_data.p16 - 1);
                IISFIFO_WR = value;
                IISFIFO_WR = value;
            }
        }
    }

    more_ready = pcm_callback_more_ready;

    if (more_ready == NULL || more_ready(0) < 0) {
        /* Finished recording */
        pcm_rec_dma_stop();
        pcm_rec_dma_stopped_callback();
    }
}

#else
void fiq_record(void)
{
    register pcm_more_callback_type2 more_ready;

    while (dma_rec_data.size > 0) {
        if (IIS_RX_FULL_COUNT < 2) {
            return;
        }

#if SAMPLE_SIZE == 16
        *dma_rec_data.p16++ = IISFIFO_RD;
#elif SAMPLE_SIZE == 32
        *dma_rec_data.p32++ = IISFIFO_RD >> 16;
        *dma_rec_data.p32++ = IISFIFO_RD >> 16;
#endif
        dma_rec_data.size -= 4;
    }

    more_ready = pcm_callback_more_ready;

    if (more_ready == NULL || more_ready(0) < 0) {
        /* Finished recording */
        pcm_rec_dma_stop();
        pcm_rec_dma_stopped_callback();
    }
}

#endif /* SANSA_E200 */

/* Continue transferring data in */
void pcm_record_more(void *start, size_t size)
{
    pcm_rec_peak_addr = start; /* Start peaking at dest */
    dma_rec_data.addr = (unsigned long)start; /* Start of RX buffer */
    dma_rec_data.size = size; /* Bytes to transfer */
}

void pcm_rec_dma_stop(void)
{
    /* disable interrupt */
    IIS_IRQRX_REG &= ~IIS_IRQRX;

    dma_rec_data.state = 0;
    dma_rec_data.size = 0;
#if NUM_CORES > 1
    dma_rec_data.core = 0x00;
#endif

    /* disable fifo */
    IISCONFIG &= ~IIS_RXFIFOEN;
    IISFIFO_CFG |= IIS_RXCLR;
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    pcm_rec_dma_stop();

    pcm_rec_peak_addr = addr;
    dma_rec_data.addr = (unsigned long)addr;
    dma_rec_data.size = size;
#if NUM_CORES > 1
    /* This will become more important later - and different ! */
    dma_rec_data.core = processor_id(); /* save initiating core */
#endif
    /* setup FIQ handler */
    fiq_function = fiq_record;

    /* interrupt on full fifo, enable record fifo interrupt */
    dma_rec_data.state = 1;

    /* enable RX FIFO */
    IISCONFIG |= IIS_RXFIFOEN;

    /* enable IIS interrupt as FIQ */
    CPU_INT_PRIORITY |= IIS_MASK;
    CPU_INT_EN = IIS_MASK;
}

void pcm_rec_dma_close(void)
{
    pcm_rec_dma_stop();
} /* pcm_close_recording */

void pcm_rec_dma_init(void)
{
    pcm_rec_dma_stop();
} /* pcm_init */

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    unsigned long addr, end;

    int status = disable_fiq_save();
    addr = (unsigned long)pcm_rec_peak_addr;
    end = dma_rec_data.addr;
    restore_fiq(status);

    *count = (end >> 2) - (addr >> 2);
    return (void *)(addr & ~3);
} /* pcm_rec_dma_get_peak_buffer */

#endif /* HAVE_RECORDING */
