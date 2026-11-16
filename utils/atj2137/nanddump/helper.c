/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * Raw NAND page reader called by hwstub. Same command sequences as
 * hwstub/tools/lua/atj/nand.lua (see there for command word format).
 * Only read commands are ever issued.
 *
 ****************************************************************************/
#include <stdint.h>
#include "helper.h"

#define REG32(a)        (*(volatile uint32_t *)(a))

#define CMU_NANDCLK     REG32(0xb0010018)
#define CMU_DEVCLKEN    REG32(0xb0010080)
#define CMU_DEVRST      REG32(0xb0010084)
#define GPIO_MFCTL0     REG32(0xb01c0018)
#define GPIO_MFCTL1     REG32(0xb01c001c)

#define NAND_CTL        REG32(0xb00a0000)
#define NAND_STATUS     REG32(0xb00a0004)
#define NAND_FIFOTIM    REG32(0xb00a0008)
#define NAND_COLSPARE   REG32(0xb00a000c)
#define NAND_COLMAIN    REG32(0xb00a0010)
#define NAND_ROW        REG32(0xb00a0014)
#define NAND_ROWHI      REG32(0xb00a0018)
#define NAND_DATALEN    REG32(0xb00a001c)
#define NAND_USERDATA   REG32(0xb00a0020)
#define NAND_FSM        REG32(0xb00a0024)
#define NAND_FIFO       REG32(0xb00a0028)
#define NAND_ECCMODE    REG32(0xb00a002c)
#define NAND_FIFO_PHYS  0x100a0028

#define DMAC_CTL        REG32(0xb0060000)
#define DMAC_IRQPD      REG32(0xb0060008)
#define DMA_CH          1
#define DMA_MODE        REG32(0xb0060100 + 0x20 * DMA_CH)
#define DMA_SRC         REG32(0xb0060104 + 0x20 * DMA_CH)
#define DMA_DST         REG32(0xb0060108 + 0x20 * DMA_CH)
#define DMA_CNT         REG32(0xb006010c + 0x20 * DMA_CH)
#define DMA_CMD         REG32(0xb0060114 + 0x20 * DMA_CH)

#define TIMEOUT         1000000
#define FILL_PATTERN    0x5aa5c33c

static uint32_t error_status;

static void udelay(unsigned us)
{
    /* rough, core runs at 36MHz under hwstub */
    volatile unsigned n = us * 12;
    while (n--)
    {
    }
}

static int wait_fsm(void)
{
    unsigned n = TIMEOUT;
    while (NAND_STATUS & 0x80000000)
    {
        if (!--n)
        {
            error_status = NAND_STATUS;
            return ST_TIMEOUT_FSM;
        }
    }
    return ST_OK;
}

static int wait_xfer(void)
{
    unsigned n = TIMEOUT;
    while (NAND_STATUS & 0x80)
    {
        if (!--n)
        {
            error_status = NAND_STATUS;
            return ST_TIMEOUT_XFER;
        }
    }
    return ST_OK;
}

/* NAND R/B line, STATUS[5] = 1 when ready */
static int wait_ready(void)
{
    unsigned n = TIMEOUT;
    udelay(2); /* tWB, R/B goes low only some time after 30h */
    while (!(NAND_STATUS & 0x20))
    {
        if (!--n)
        {
            error_status = NAND_STATUS;
            return ST_TIMEOUT_RB;
        }
    }
    return ST_OK;
}

static int wait_dma(void)
{
    unsigned n = TIMEOUT;
    while (DMA_CMD & 1)
    {
        if (!--n)
        {
            error_status = DMA_CNT;
            return ST_TIMEOUT_DMA;
        }
    }
    return ST_OK;
}

static void nand_select(uint32_t ce)
{
    NAND_CTL = (NAND_CTL & 0xffffff86) | (1 << (ce + 4)) | 1;
}

static void nand_deselect(void)
{
    NAND_CTL &= 0xffffff86;
}

static int nand_init(struct helper_params_t *p)
{
    CMU_DEVCLKEN |= 0x4000314;
    CMU_DEVRST &= ~0x20;
    udelay(10);
    CMU_DEVRST |= 0x20;
    udelay(10);
    CMU_NANDCLK = p->nandclk;
    udelay(10);
    GPIO_MFCTL0 = (GPIO_MFCTL0 & 0xfe3ff300) | 0x400449;
    GPIO_MFCTL1 |= 0x80000000;
    NAND_FIFOTIM = p->fifotim;
    udelay(10);
    NAND_CTL = 0x40903;
    NAND_ECCMODE = 1;
    NAND_COLSPARE = 0;
    NAND_COLMAIN = 0;
    NAND_ROW = 0;
    NAND_ROWHI = 0;
    NAND_USERDATA = 0;
    NAND_DATALEN = 0;

    /* RESET (FFh) the chip, as the working Lua sequence does */
    nand_select(p->ce);
    int ret = wait_fsm();
    if (ret == ST_OK)
    {
        NAND_FSM = 0x100ff;
        (void)NAND_FSM;
        ret = wait_fsm();
        udelay(5000);
    }
    nand_deselect();
    return ret;
}

static int nand_readid(struct helper_params_t *p)
{
    int ret;

    nand_select(p->ce);
    if ((ret = wait_fsm()))
    {
        goto out;
    }
    NAND_ROW = 0;
    NAND_DATALEN = 4;
    NAND_FSM = 0x190090;
    (void)NAND_FSM;
    if ((ret = wait_xfer()))
    {
        goto out;
    }
    *(volatile uint32_t *)HELPER_BUF = NAND_FIFO;
    (void)NAND_FSM;
    ret = wait_fsm();
out:
    nand_deselect();
    return ret;
}

/* channel is only programmed here, it is started after the FSM command
 * words are issued, exactly as BROM does */
static void dma_setup(uint32_t dst, uint32_t len)
{
    DMAC_CTL = 0x10000 << DMA_CH; /* reset channel */
    DMAC_CTL = 1 << DMA_CH;
    DMAC_IRQPD = 0xffffffff;
    /* mode words from BROM: FIFO -> SDRAM / IRAM */
    DMA_MODE = (dst & 0x10000000) ? 0x8c01a4 : 0x8401a4;
    DMA_SRC = NAND_FIFO_PHYS;
    DMA_DST = dst & 0x1fffffff;
    DMA_CNT = len;
}

static uint32_t load_delay;

static int read_page(uint32_t row, uint32_t dst, uint32_t len, uint32_t chunk)
{
    uint32_t done, n;
    int ret;

    if ((ret = wait_fsm()))
    {
        return ret;
    }

    /* 00h col row 30h: page to chip cache, wait R/B */
    NAND_COLMAIN = 0;
    NAND_ROW = row;
    NAND_ROWHI = 0;
    NAND_FSM = 0x80b10000;
    NAND_FSM = 0x50030;
    /* FSM busy flag is not raised immediately, give it time to send 30h
     * and the chip time to load the page before anything is polled */
    udelay(load_delay);
    (void)NAND_FSM;
    if ((ret = wait_fsm()))
    {
        return ret;
    }
    /* FSM idle does not imply page is in cache yet (MLC upper pages
     * have long tR), wait for R/B explicitly */
    if ((ret = wait_ready()))
    {
        return ret;
    }

    /* mark destination so data not written by DMA can't pass as NAND data */
    for (done = 0; done < len; done += 4)
    {
        *(volatile uint32_t *)(dst + done) = FILL_PATTERN;
    }

    /* 05h col E0h: random data out, chunk by chunk */
    for (done = 0; done < len; done += n)
    {
        n = len - done < chunk ? len - done : chunk;
        dma_setup(dst + done, n);
        NAND_DATALEN = n;
        NAND_COLMAIN = done;
        NAND_FSM = 0x80810005;
        NAND_FSM = 0x900e0;
        DMA_CMD = 1;
        if ((ret = wait_dma()))
        {
            return ret;
        }
        (void)NAND_FSM;
        if ((ret = wait_fsm()))
        {
            return ret;
        }
    }

    return ST_OK;
}

static int nand_read(struct helper_params_t *p)
{
    uint32_t i;
    int ret = ST_OK;

    if (p->page_bytes == 0 || (p->page_bytes & 3) || p->chunk == 0 ||
        (p->chunk & 3) || p->page_bytes * p->npages > HELPER_BUF_SIZE)
    {
        return ST_BADPARAM;
    }

    NAND_ECCMODE = p->eccmode;
    load_delay = p->load_delay;
    for (i = 0; i < p->npages; i++)
    {
        /* CE is toggled around every page like in the Lua reader */
        nand_select(p->ce);
        ret = read_page(p->page + i, HELPER_BUF + i * p->page_bytes,
                        p->page_bytes, p->chunk);
        nand_deselect();
        if (ret)
        {
            break;
        }
        p->done = i + 1;
    }
    return ret;
}

void helper_main(struct helper_params_t *p)
{
    int ret;

    p->done = 0;
    p->dbg = 0;
    error_status = 0;

    if (p->magic != HELPER_MAGIC)
    {
        p->status = ST_BADMAGIC;
        return;
    }

    switch (p->op)
    {
        case OP_INIT:   ret = nand_init(p); break;
        case OP_READID: ret = nand_readid(p); break;
        case OP_READ:   ret = nand_read(p); break;
        default:        ret = ST_BADOP; break;
    }

    p->dbg = error_status;
    p->status = ret;
}
