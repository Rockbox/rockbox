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
#include "panic.h"
#include "sfc-x1000.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "x1000/sfc.h"
#include "x1000/cpm.h"

#ifndef BOOTLOADER_SPL
/* DMA only works once the system is properly booted */
# define NEED_SFC_DMA
#endif

#if defined(BOOTLOADER_SPL)
# if X1000_EXCLK_FREQ == 24000000
#  define FIXED_CLK_FREQ  600000000
#  define FIXED_CLK_SRC   X1000_CLK_MPLL
# elif X1000_EXCLK_FREQ == 26000000
#  define FIXED_CLK_FREQ  598000000
#  define FIXED_CLK_SRC   X1000_CLK_MPLL
# else
#  error "bad EXCLK freq"
# endif
#endif

#define FIFO_THRESH 31

#define SFC_STATUS_PENDING (-1)

#ifdef NEED_SFC_DMA
static struct mutex sfc_mutex;
static struct semaphore sfc_sema;
static struct timeout sfc_lockup_tmo;
static bool sfc_inited = false;
static volatile int sfc_status;
#else
# define sfc_status SFC_STATUS_OK
#endif

void sfc_init(void)
{
#ifdef NEED_SFC_DMA
    if(sfc_inited)
        return;

    mutex_init(&sfc_mutex);
    semaphore_init(&sfc_sema, 1, 0);
    sfc_inited = true;
#endif
}

void sfc_lock(void)
{
#ifdef NEED_SFC_DMA
    mutex_lock(&sfc_mutex);
#endif
}

void sfc_unlock(void)
{
#ifdef NEED_SFC_DMA
    mutex_unlock(&sfc_mutex);
#endif
}

void sfc_open(void)
{
    gpio_config(GPIO_A, 0x3f << 26, GPIO_DEVICE(1));
    jz_writef(CPM_CLKGR, SFC(0));
    jz_writef(SFC_GLB, OP_MODE_V(SLAVE), PHASE_NUM(1),
              THRESHOLD(FIFO_THRESH), WP_EN(1));
    REG_SFC_CGE = 0;
    REG_SFC_INTC = 0x1f;
    REG_SFC_MEM_ADDR = 0;

#ifdef NEED_SFC_DMA
    jz_writef(SFC_GLB, OP_MODE_V(DMA), BURST_MD_V(INCR32));
    system_enable_irq(IRQ_SFC);
#endif
}

void sfc_close(void)
{
#ifdef NEED_SFC_DMA
    system_disable_irq(IRQ_SFC);
#endif

    REG_SFC_CGE = 0x1f;
    jz_writef(CPM_CLKGR, SFC(1));
}

void sfc_set_clock(x1000_clk_t clksrc, uint32_t freq)
{
    uint32_t in_freq;
#ifdef FIXED_CLK_FREQ
    /* Small optimization to save code space in SPL by not polling clock */
    clksrc = FIXED_CLK_SRC;
    in_freq = FIXED_CLK_FREQ;
#else
    in_freq = clk_get(clksrc);
#endif

    uint32_t div = clk_calc_div(in_freq, freq);
    jz_writef(CPM_SSICDR, CE(1), CLKDIV(div - 1),
              SFC_CS(clksrc == X1000_CLK_MPLL ? 1 : 0));
    while(jz_readf(CPM_SSICDR, BUSY));
    jz_writef(CPM_SSICDR, CE(0));
}

#ifdef NEED_SFC_DMA
static int sfc_lockup_tmo_cb(struct timeout* tmo)
{
    (void)tmo;

    int irq = disable_irq_save();
    if(sfc_status == SFC_STATUS_PENDING) {
        sfc_status = SFC_STATUS_LOCKUP;
        jz_overwritef(SFC_TRIG, STOP(1));
        semaphore_release(&sfc_sema);
    }

    restore_irq(irq);
    return 0;
}

static void sfc_wait_end(void)
{
    semaphore_wait(&sfc_sema, TIMEOUT_BLOCK);
}

void SFC(void)
{
    unsigned sr = REG_SFC_SR & ~REG_SFC_INTC;

    if(jz_vreadf(sr, SFC_SR, OVER)) {
        jz_overwritef(SFC_SCR, CLR_OVER(1));
        sfc_status = SFC_STATUS_OVERFLOW;
    } else if(jz_vreadf(sr, SFC_SR, UNDER)) {
        jz_overwritef(SFC_SCR, CLR_UNDER(1));
        sfc_status = SFC_STATUS_UNDERFLOW;
    } else if(jz_vreadf(sr, SFC_SR, END)) {
        jz_overwritef(SFC_SCR, CLR_END(1));
        sfc_status = SFC_STATUS_OK;
    } else {
        panicf("SFC IRQ bug");
        return;
    }

    /* Not sure this is wholly correct */
    if(sfc_status != SFC_STATUS_OK)
        jz_overwritef(SFC_TRIG, STOP(1));

    REG_SFC_INTC = 0x1f;
    semaphore_release(&sfc_sema);
}
#else
/* Note the X1000 is *very* picky about how the SFC FIFOs are accessed
 * so please do NOT try to rearrange the code without testing it first!
 */

void sfc_fifo_read(unsigned* buffer, int data_bytes)
{
    int data_words = (data_bytes + 3) / 4;
    while(data_words > 0) {
        if(jz_readf(SFC_SR, RREQ)) {
            jz_overwritef(SFC_SCR, CLR_RREQ(1));

            int amount = data_words > FIFO_THRESH ? FIFO_THRESH : data_words;
            data_words -= amount;
            while(amount > 0) {
                *buffer++ = REG_SFC_DATA;
                amount -= 1;
            }
        }
    }
}

void sfc_fifo_write(const unsigned* buffer, int data_bytes)
{
    int data_words = (data_bytes + 3) / 4;
    while(data_words > 0) {
        if(jz_readf(SFC_SR, TREQ)) {
            jz_overwritef(SFC_SCR, CLR_TREQ(1));

            int amount = data_words > FIFO_THRESH ? FIFO_THRESH : data_words;
            data_words -= amount;
            while(amount > 0) {
                REG_SFC_DATA = *buffer++;
                amount -= 1;
            }
        }
    }
}

static void sfc_wait_end(void)
{
    while(jz_readf(SFC_SR, END) == 0);
    jz_overwritef(SFC_SCR, CLR_TREQ(1));
}

#endif /* NEED_SFC_DMA */

int sfc_exec(const sfc_op* op)
{
#ifdef NEED_SFC_DMA
    uint32_t intc_clear = jz_orm(SFC_INTC, MSK_END);
#endif

    if(op->flags & (SFC_FLAG_READ|SFC_FLAG_WRITE)) {
        jz_writef(SFC_TRAN_CONF(0), DATA_EN(1));
        REG_SFC_TRAN_LENGTH = op->data_bytes;
#ifdef NEED_SFC_DMA
        REG_SFC_MEM_ADDR = PHYSADDR(op->buffer);
#endif

        if(op->flags & SFC_FLAG_READ)
        {
            jz_writef(SFC_GLB, TRAN_DIR_V(READ));
#ifdef NEED_SFC_DMA
            discard_dcache_range(op->buffer, op->data_bytes);
            intc_clear |= jz_orm(SFC_INTC, MSK_OVER);
#endif
        }
        else
        {
            jz_writef(SFC_GLB, TRAN_DIR_V(WRITE));
#ifdef NEED_SFC_DMA
            commit_dcache_range(op->buffer, op->data_bytes);
            intc_clear |= jz_orm(SFC_INTC, MSK_UNDER);
#endif
        }
    } else {
        jz_writef(SFC_TRAN_CONF(0), DATA_EN(0));
        REG_SFC_TRAN_LENGTH = 0;
#ifdef NEED_SFC_DMA
        REG_SFC_MEM_ADDR = 0;
#endif
    }

    bool dummy_first = (op->flags & SFC_FLAG_DUMMYFIRST) != 0;
    jz_writef(SFC_TRAN_CONF(0),
              MODE(op->mode), POLL_EN(0),
              ADDR_WIDTH(op->addr_bytes),
              PHASE_FMT(dummy_first ? 1 : 0),
              DUMMY_BITS(op->dummy_bits),
              COMMAND(op->command), CMD_EN(1));

    REG_SFC_DEV_ADDR(0) = op->addr_lo;
    REG_SFC_DEV_PLUS(0) = op->addr_hi;

#ifdef NEED_SFC_DMA
    sfc_status = SFC_STATUS_PENDING;
    timeout_register(&sfc_lockup_tmo, sfc_lockup_tmo_cb, 10*HZ, 0);
    REG_SFC_SCR = 0x1f;
    REG_SFC_INTC &= ~intc_clear;
#endif

    jz_overwritef(SFC_TRIG, FLUSH(1));
    jz_overwritef(SFC_TRIG, START(1));

#ifndef NEED_SFC_DMA
    if(op->flags & SFC_FLAG_READ)
        sfc_fifo_read((unsigned*)op->buffer, op->data_bytes);
    if(op->flags & SFC_FLAG_WRITE)
        sfc_fifo_write((const unsigned*)op->buffer, op->data_bytes);
#endif

    sfc_wait_end();

#ifdef NEED_SFC_DMA
    if(op->flags & SFC_FLAG_READ)
        discard_dcache_range(op->buffer, op->data_bytes);
#endif

    return sfc_status;
}
