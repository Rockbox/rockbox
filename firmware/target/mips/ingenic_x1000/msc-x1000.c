/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2021-2026 Aidan MacDonald
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
#include "panic.h"
#include "led.h"
#include "msc-x1000.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "clk-x1000.h"
#include "x1000/msc.h"
#include "x1000/cpm.h"
#include <string.h>
#include <stddef.h>

/*
 * TODO: Clock helpers could be factored out of this file
 */

static uint32_t get_msc_clkgr_bit(int msc)
{
    if (msc == 0)
        return BM_CPM_CLKGR_MSC0;
    else
        return BM_CPM_CLKGR_MSC1;
}

static bool msc_is_gated(int msc)
{
    return REG_CPM_CLKGR & get_msc_clkgr_bit(msc);
}

static void msc_gate_clock(int msc, bool gate)
{
    if (gate)
        REG_CPM_CLKGR |= get_msc_clkgr_bit(msc);
    else
        REG_CPM_CLKGR &= ~get_msc_clkgr_bit(msc);
}

static void msc_set_clock(int msc, uint32_t src_freq, uint32_t bus_freq)
{
    /* Wait for clock to go idle from any ongoing operation */
    while (jz_readf(MSC_STAT(msc), CLOCK_EN))
        sleep(1);

    /*
     * Handle MSCxDIV, the max CLKRT division factor is 1/128
     * which allows us to get a 400 KHz clock from up to 50 MHz
     * so there is no point using a lower MSCxDIV clock.
     */
    uint32_t mscdiv_freq = MAX(bus_freq, 50000000);
    uint32_t div = clk_calc_div(src_freq / 2, mscdiv_freq);
    if (msc == 0)
    {
        jz_writef(CPM_MSC0CDR, CE(1), CLKDIV(div - 1));
        while(jz_readf(CPM_MSC0CDR, BUSY));
        jz_writef(CPM_MSC0CDR, CE(0));
    }
    else
    {
        jz_writef(CPM_MSC1CDR, CE(1), CLKDIV(div - 1));
        while(jz_readf(CPM_MSC1CDR, BUSY));
        jz_writef(CPM_MSC1CDR, CE(0));
    }

    /* Handle MSC_CLKRT */
    uint32_t clkrt = clk_calc_shift(src_freq / (2 * div), bus_freq);
    REG_MSC_CLKRT(msc) = clkrt;

    /*
     * Handle frequency dependent timing settings
     * TODO - these settings might be SD specific...
     */
    uint32_t out_freq = (src_freq / (2 * div)) >> clkrt;
    if (out_freq > 25000000)
    {
        jz_writef(MSC_LPM(msc),
                  DRV_SEL_V(RISE_EDGE_DELAY_QTR_PHASE),
                  SMP_SEL_V(RISE_EDGE_DELAYED));
        jz_writef(MSC_CTRL2(msc), SPEED_V(HIGHSPEED));
    }
    else
    {
        jz_writef(MSC_LPM(msc),
                  DRV_SEL_V(FALL_EDGE),
                  SMP_SEL_V(RISE_EDGE));
        jz_writef(MSC_CTRL2(msc), SPEED_V(DEFAULT));
    }
}

void x1000_msc_init(struct x1000_msc_controller* ctl,
                    struct x1000_msc_dma_desc *dma_desc,
                    int msc_nr, uint32_t src_clk_freq)
{
    memset(ctl, 0, sizeof(*ctl));

    ctl->msc_nr = msc_nr;
    ctl->src_clk_freq = src_clk_freq;
    ctl->dma_desc = dma_desc;
    ctl->cmdat_def = jz_orf(MSC_CMDAT, RTRG_V(GE32), TTRG_V(LE32), INIT(1));

    semaphore_init(&ctl->sem, 1, 0);

    /* Start off gated, sdmmc_host will power us on */
    msc_gate_clock(ctl->msc_nr, true);
}

void x1000_msc_set_power_enabled(void *controller, bool enabled)
{
    struct x1000_msc_controller *ctl = controller;

    if (enabled)
    {
        msc_gate_clock(ctl->msc_nr, false);

        /* Controller reset */
        jz_overwritef(MSC_CTRL(ctl->msc_nr), RESET(1));
        udelay(100);
        jz_writef(MSC_CTRL(ctl->msc_nr), RESET(0));
        while (jz_readf(MSC_STAT(ctl->msc_nr), IS_RESETTING));

        /* Ensure interrupt state is clear */
        REG_MSC_IMASK(ctl->msc_nr) = 0xFFFFFFFFu;
        REG_MSC_IFLAG(ctl->msc_nr) = 0xFFFFFFFFu;

        /* Set requested bus clock frequency */
        msc_set_clock(ctl->msc_nr, ctl->src_clk_freq,
                      sdmmc_host_get_bus_freq(ctl->bus_clock));

        /* Set INIT bit and enable auto clock management */
        ctl->cmdat_def |= BM_MSC_CMDAT_INIT;
        jz_writef(MSC_LPM(ctl->msc_nr), ENABLE(1));
    }
    else
    {
        /* Disable and gate clock */
        jz_writef(MSC_LPM(ctl->msc_nr), ENABLE(0));
        jz_writef(MSC_CTRL(ctl->msc_nr), CLOCK_V(STOP));

        msc_gate_clock(ctl->msc_nr, true);
    }
}

void x1000_msc_set_bus_width(void *controller, uint32_t width)
{
    struct x1000_msc_controller *ctl = controller;

    switch (width)
    {
    case SDMMC_BUS_WIDTH_1BIT:
        jz_vwritef(ctl->cmdat_def, MSC_CMDAT, BUS_WIDTH_V(1BIT));
        break;

    case SDMMC_BUS_WIDTH_4BIT:
        jz_vwritef(ctl->cmdat_def, MSC_CMDAT, BUS_WIDTH_V(4BIT));
        break;

    case SDMMC_BUS_WIDTH_8BIT:
        jz_vwritef(ctl->cmdat_def, MSC_CMDAT, BUS_WIDTH_V(8BIT));
        break;

    default:
        panicf("%s", __func__);
    }
}

void x1000_msc_set_bus_clock(void *controller, uint32_t clock)
{
    struct x1000_msc_controller *ctl = controller;
    uint32_t bus_freq = sdmmc_host_get_bus_freq(clock);

    ctl->bus_clock = clock;

    if (!msc_is_gated(ctl->msc_nr))
        msc_set_clock(ctl->msc_nr, ctl->src_clk_freq, bus_freq);
}

int x1000_msc_submit_command(void *controller,
                             const struct sdmmc_host_command *cmd,
                             struct sdmmc_host_response *resp)
{
    struct x1000_msc_controller *ctl = controller;
    uint32_t cmdat = ctl->cmdat_def;
    uint32_t imask = jz_orm(MSC_IMASK,
                            CRC_RES_ERROR, CRC_READ_ERROR, CRC_WRITE_ERROR,
                            TIME_OUT_RES, TIME_OUT_READ, END_CMD_RES);

    void *buff_addr = cmd->buffer;
    size_t buff_size = cmd->nr_blocks * cmd->block_len;

    /* INIT is only sent for the first command after power up */
    jz_vwritef(ctl->cmdat_def, MSC_CMDAT, INIT(0));

    /* Response type setting */
    if (cmd->flags & SDMMC_RESP_BUSY)
        jz_vwritef(cmdat, MSC_CMDAT, BUSY(1));

    switch (SDMMC_RESP_LENGTH(cmd->flags))
    {
    case SDMMC_RESP_NONE:
        jz_vwritef(cmdat, MSC_CMDAT, RESP_FMT(0));

        ctl->resp_len = 0;
        break;

    case SDMMC_RESP_SHORT:
        if (cmd->flags & SDMMC_RESP_NOCRC)
            jz_vwritef(cmdat, MSC_CMDAT, RESP_FMT(3));
        else
            jz_vwritef(cmdat, MSC_CMDAT, RESP_FMT(1));

        ctl->resp_len = 1;
        break;

    case SDMMC_RESP_LONG:
        jz_vwritef(cmdat, MSC_CMDAT, RESP_FMT(2));
        ctl->resp_len = 4;
        break;

    default:
        panicf("%s", __func__);
        break;
    }

    /* Data transfer setup */
    if (SDMMC_DATA_PRESENT(cmd->flags))
    {
        if ((uintptr_t)buff_addr & (CACHEALIGN_SIZE - 1))
            panicf("%s: unaligned buffer", __func__);

        ctl->dma_desc->nda = 0;
        ctl->dma_desc->mem = PHYSADDR(buff_addr);
        ctl->dma_desc->len = buff_size;
        ctl->dma_desc->cmd = 2; /* ID=0, ENDI=1, LINK=0 */
        commit_dcache_range(ctl->dma_desc, sizeof(*ctl->dma_desc));

        if (SDMMC_DATA_DIR(cmd->flags) == SDMMC_DATA_WRITE)
        {
            commit_dcache_range(buff_addr, buff_size);

            jz_vwritef(cmdat, MSC_CMDAT, WRITE_READ(1));
            ctl->iflag_done = jz_orm(MSC_IMASK, WR_ALL_DONE);
        }
        else
        {
            discard_dcache_range(buff_addr, buff_size);

            ctl->iflag_done = jz_orm(MSC_IMASK, DMA_DATA_DONE);
        }

        jz_vwritef(cmdat, MSC_CMDAT, DATA_EN(1));

        jz_writef(MSC_DMAC(ctl->msc_nr), MODE_SEL(0), INCR(0), DMASEL(0));
        REG_MSC_DMANDA(ctl->msc_nr) = PHYSADDR(ctl->dma_desc);
    }
    else
    {
        /*
         * PROG_DONE is actually waiting for the busy signal so is
         * required for all commands with R1b response (like CMD12).
         * For writes, the WR_ALL_DONE interrupt subsumes PROG_DONE.
         */
        if (cmd->flags & SDMMC_RESP_BUSY)
            ctl->iflag_done = jz_orm(MSC_IMASK, PROG_DONE);
        else
            ctl->iflag_done = jz_orm(MSC_IMASK, END_CMD_RES);
    }

    REG_MSC_NOB(ctl->msc_nr) = cmd->nr_blocks;
    REG_MSC_BLKLEN(ctl->msc_nr) = cmd->block_len;
    REG_MSC_CMD(ctl->msc_nr) = cmd->command;
    REG_MSC_ARG(ctl->msc_nr) = cmd->argument;
    REG_MSC_CMDAT(ctl->msc_nr) = cmdat;

    imask |= ctl->iflag_done;
    REG_MSC_IFLAG(ctl->msc_nr) = imask;
    REG_MSC_IMASK(ctl->msc_nr) &= ~imask;

    ctl->resp = resp;
    ctl->err_code = SDMMC_STATUS_OK;
    membarrier();

    jz_writef(MSC_CTRL(ctl->msc_nr), START_OP(1));
    if (SDMMC_DATA_PRESENT(cmd->flags))
        jz_writef(MSC_DMAC(ctl->msc_nr), ENABLE(1));

    semaphore_wait(&ctl->sem, TIMEOUT_BLOCK);

    if (SDMMC_DATA_PRESENT(cmd->flags) &&
        SDMMC_DATA_DIR(cmd->flags) == SDMMC_DATA_READ)
        discard_dcache_range(buff_addr, buff_size);

    return ctl->err_code;
}

static void x1000_msc_finish_command(struct x1000_msc_controller *ctl)
{
    REG_MSC_IMASK(ctl->msc_nr) = 0xFFFFFFFFu;
    REG_MSC_IFLAG(ctl->msc_nr) = 0xFFFFFFFFu;
    jz_writef(MSC_DMAC(ctl->msc_nr), ENABLE(0));
    semaphore_release(&ctl->sem);
}

void x1000_msc_abort_command(void *controller)
{
    struct x1000_msc_controller *ctl = controller;
    int irq = disable_irq_save();

    /*
     * Looks strange but this wait is always OK because it either
     * acquires the semaphore of an idle controller or is a no-op
     * if there is a command running.
     *
     * Finishing the command then releases the semaphore which is
     * correct for both cases.
     */
    semaphore_wait(&ctl->sem, TIMEOUT_NOBLOCK);

    ctl->err_code = SDMMC_STATUS_ERROR;
    x1000_msc_finish_command(ctl);

    restore_irq(irq);
}

static void x1000_msc_read_response(struct x1000_msc_controller *ctl)
{
    uint32_t res = REG_MSC_RES(ctl->msc_nr);
    uint32_t dat;

    if (ctl->resp_len == 4)
    {
        for (int i = 0; i < 4; ++i)
        {
            dat = res << 24;
            res = REG_MSC_RES(ctl->msc_nr);
            dat |= res << 8;
            res = REG_MSC_RES(ctl->msc_nr);
            dat |= res >> 8;

            if (ctl->resp)
                ctl->resp->data[i] = dat;
        }
    }
    else if (ctl->resp_len == 1)
    {
        dat = res << 24;
        res = REG_MSC_RES(ctl->msc_nr);
        dat |= res << 8;
        res = REG_MSC_RES(ctl->msc_nr);
        dat |= res & 0xff;

        if (ctl->resp)
            ctl->resp->data[0] = dat;
    }
}

void x1000_msc_irq_handler(struct x1000_msc_controller *ctl)
{
    const uint32_t tmo_bits = jz_orm(MSC_IFLAG, TIME_OUT_READ, TIME_OUT_RES);
    const uint32_t crc_bits = jz_orm(MSC_IFLAG, CRC_RES_ERROR, CRC_READ_ERROR, CRC_WRITE_ERROR);

    uint32_t iflag = REG_MSC_IFLAG(ctl->msc_nr);

    if (iflag & tmo_bits)
        ctl->err_code = SDMMC_STATUS_TIMEOUT;
    else if (iflag & crc_bits)
        ctl->err_code = SDMMC_STATUS_INVALID_CRC;

    /* Read and clear command response */
    if (iflag & BM_MSC_IFLAG_END_CMD_RES)
    {
        REG_MSC_IMASK(ctl->msc_nr) |= BM_MSC_IFLAG_END_CMD_RES;
        REG_MSC_IFLAG(ctl->msc_nr) = BM_MSC_IFLAG_END_CMD_RES;
        x1000_msc_read_response(ctl);
    }

    if ((iflag & ctl->iflag_done) || ctl->err_code)
        x1000_msc_finish_command(ctl);
}
