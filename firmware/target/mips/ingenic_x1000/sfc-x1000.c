/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "system.h"
#include "kernel.h"
#include "sfc-x1000.h"
#include "clk-x1000.h"
#include "irq-x1000.h"

/* #define USE_DMA */
#define FIFO_THRESH 31

static void sfc_poll_wait(void);
#ifdef USE_DMA
static void sfc_irq_wait(void);

static struct semaphore sfc_sema;

/* This function pointer thing is a hack for the SPL, since it has to use
 * the NAND driver directly and we can't afford to drag in the whole kernel
 * just to wait on a semaphore. */
static void(*sfc_wait)(void) = sfc_poll_wait;
#endif

void sfc_open(void)
{
    jz_writef(CPM_CLKGR, SFC(0));
#ifdef USE_DMA
    jz_writef(SFC_GLB, OP_MODE_V(DMA), BURST_MD_V(INCR32),
              PHASE_NUM(1), THRESHOLD(FIFO_THRESH), WP_EN(1));
#else
    jz_writef(SFC_GLB, OP_MODE_V(SLAVE), PHASE_NUM(1),
              THRESHOLD(FIFO_THRESH), WP_EN(1));
#endif
    REG_SFC_CGE = 0;
    REG_SFC_INTC = 0x1f;
    REG_SFC_MEM_ADDR = 0;
}

void sfc_close(void)
{
    REG_SFC_CGE = 0x1f;
    jz_writef(CPM_CLKGR, SFC(1));
}

void sfc_irq_begin(void)
{
#ifdef USE_DMA
    static bool inited = false;
    if(!inited) {
        semaphore_init(&sfc_sema, 1, 0);
        inited = true;
    }

    system_enable_irq(IRQ_SFC);
    sfc_wait = sfc_irq_wait;
#endif
}

void sfc_irq_end(void)
{
#ifdef USE_DMA
    system_disable_irq(IRQ_SFC);
    sfc_wait = sfc_poll_wait;
#endif
}

void sfc_set_clock(uint32_t freq)
{
    /* FIXME: Get rid of this hack & allow defining a real clock tree... */
    x1000_clk_t clksrc = X1000_CLK_MPLL;
    uint32_t in_freq = clk_get(clksrc);
    if(in_freq < freq) {
        clksrc = X1000_CLK_SCLK_A;
        in_freq = clk_get(clksrc);
    }

    uint32_t div = clk_calc_div(in_freq, freq);
    jz_writef(CPM_SSICDR, CE(1), CLKDIV(div - 1),
              SFC_CS(clksrc == X1000_CLK_MPLL ? 1 : 0));
    while(jz_readf(CPM_SSICDR, BUSY));
    jz_writef(CPM_SSICDR, CE(0));
}

#ifndef USE_DMA
static void sfc_fifo_rdwr(bool write, void* buffer, uint32_t data_bytes)
{
    uint32_t* word_buf = (uint32_t*)buffer;
    uint32_t sr_bit = write ? BM_SFC_SR_TREQ : BM_SFC_SR_RREQ;
    uint32_t clr_bit = write ? BM_SFC_SCR_CLR_TREQ : BM_SFC_SCR_CLR_RREQ;
    uint32_t data_words = (data_bytes + 3) / 4;
    while(data_words > 0) {
        if(REG_SFC_SR & sr_bit) {
            REG_SFC_SCR = clr_bit;

            /* We need to read/write in bursts equal to FIFO threshold amount
             * X1000 PM, 10.8.5, SFC > software guidelines > slave mode */
            uint32_t amount = MIN(data_words, FIFO_THRESH);
            data_words -= amount;

            uint32_t* endptr = word_buf + amount;
            for(; word_buf != endptr; ++word_buf) {
                if(write)
                    REG_SFC_DATA = *word_buf;
                else
                    *word_buf = REG_SFC_DATA;
            }
        }
    }
}
#endif

void sfc_exec(uint32_t cmd, uint32_t addr, void* data, uint32_t size)
{
    /* Deal with transfer direction */
    bool write = (size & SFC_WRITE) != 0;
    uint32_t glb = REG_SFC_GLB;
    if(data) {
        if(write) {
            jz_vwritef(glb, SFC_GLB, TRAN_DIR_V(WRITE));
            size &= ~SFC_WRITE;
#ifdef USE_DMA
            commit_dcache_range(data, size);
#endif
        } else {
            jz_vwritef(glb, SFC_GLB, TRAN_DIR_V(READ));
#ifdef USE_DMA
            discard_dcache_range(data, size);
#endif
        }
    }

    /* Program transfer configuration */
    REG_SFC_GLB = glb;
    REG_SFC_TRAN_LENGTH = size;
#ifdef USE_DMA
    REG_SFC_MEM_ADDR = PHYSADDR(data);
#endif
    REG_SFC_TRAN_CONF(0) = cmd;
    REG_SFC_DEV_ADDR(0) = addr;
    REG_SFC_DEV_PLUS(0) = 0;

    /* Clear old interrupts */
    REG_SFC_SCR = 0x1f;
    jz_writef(SFC_INTC, MSK_END(0));

    /* Start the command */
    jz_overwritef(SFC_TRIG, FLUSH(1));
    jz_overwritef(SFC_TRIG, START(1));

    /* Data transfer by PIO or DMA, and wait for completion */
#ifndef USE_DMA
    sfc_fifo_rdwr(write, data, size);
    sfc_poll_wait();
#else
    sfc_wait();
#endif
}

static void sfc_poll_wait(void)
{
    while(jz_readf(SFC_SR, END) == 0);
    jz_overwritef(SFC_SCR, CLR_END(1));
}

#ifdef USE_DMA
static void sfc_irq_wait(void)
{
    semaphore_wait(&sfc_sema, TIMEOUT_BLOCK);
}

void SFC(void)
{
    /* the only interrupt we use is END; errors are basically not
     * possible with the SPI interface... */
    semaphore_release(&sfc_sema);
    jz_overwritef(SFC_SCR, CLR_END(1));
    jz_writef(SFC_INTC, MSK_END(1));
}
#endif
