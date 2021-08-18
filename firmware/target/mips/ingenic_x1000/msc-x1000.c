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

/* #define LOGF_ENABLE */
#include "logf.h"

/* TODO - this needs some auditing to better handle errors
 *
 * There should be a clearer code path involving errors. Especially we should
 * ensure that removing the card always resets the driver to a sane state.
 */

#define DEBOUNCE_TIME (HZ/10)

static const msc_config msc_configs[] = {
#if defined(FIIO_M3K)
#define MSC_CLOCK_SOURCE X1000_CLK_SCLK_A
    {
        .msc_nr = 0,
        .msc_type = MSC_TYPE_SD,
        .bus_width = 4,
        .label = "microSD",
        .cd_gpio = GPIO_MSC0_CD,
        .cd_active_level = 0,
    },
#elif defined(SHANLING_Q1)
#define MSC_CLOCK_SOURCE  X1000_CLK_MPLL
    {
        .msc_nr = 0,
        .msc_type = MSC_TYPE_SD,
        .bus_width = 4,
        .label = "microSD",
        .cd_gpio = GPIO_MSC0_CD,
        .cd_active_level = 0,
    },
    /* NOTE: SDIO wifi card is on msc1 */
#elif defined(EROS_QN)
#define MSC_CLOCK_SOURCE X1000_CLK_SCLK_A
    {
        .msc_nr = 0,
        .msc_type = MSC_TYPE_SD,
        .bus_width = 4,
        .label = "microSD",
        .cd_gpio = GPIO_MSC0_CD,
        .cd_active_level = 0,
    },
#else
# error "Please add X1000 MSC config"
#endif
    {.msc_nr = -1},
};

static const msc_config* msc_lookup_config(int msc)
{
    for(int i = 0; i < MSC_COUNT; ++i)
        if(msc_configs[i].msc_nr == msc)
            return &msc_configs[i];
    return NULL;
}

static msc_drv msc_drivers[MSC_COUNT];

static void msc0_cd_interrupt(void);
static void msc1_cd_interrupt(void);

/* ---------------------------------------------------------------------------
 * Initialization
 */

static void msc_gate_clock(int msc, bool gate)
{
    int bit;
    if(msc == 0)
        bit = BM_CPM_CLKGR_MSC0;
    else
        bit = BM_CPM_CLKGR_MSC1;

    if(gate)
        REG_CPM_CLKGR |= bit;
    else
        REG_CPM_CLKGR &= ~bit;
}

static void msc_init_one(msc_drv* d, int msc)
{
    /* Lookup config */
    d->drive_nr = -1;
    d->config = msc_lookup_config(msc);
    if(!d->config) {
        d->msc_nr = -1;
        return;
    }

    /* Initialize driver state */
    d->msc_nr = msc;
    d->driver_flags = 0;
    d->clk_status = 0;
    d->cmdat_def = jz_orf(MSC_CMDAT, RTRG_V(GE32), TTRG_V(LE32));
    d->req = NULL;
    d->iflag_done = 0;
    d->card_present = 1;
    d->card_present_last = 1;
    d->req_running = 0;
    mutex_init(&d->lock);
    semaphore_init(&d->cmd_done, 1, 0);

    /* Ensure correct clock source */
    jz_writef(CPM_MSC0CDR, CE(1), CLKDIV(0),
              CLKSRC(MSC_CLOCK_SOURCE == X1000_CLK_MPLL ? 1 : 0));
    while(jz_readf(CPM_MSC0CDR, BUSY));
    jz_writef(CPM_MSC0CDR, CE(0));

    /* Initialize the hardware */
    msc_gate_clock(msc, false);
    msc_full_reset(d);
    system_enable_irq(msc == 0 ? IRQ_MSC0 : IRQ_MSC1);

    /* Setup the card detect IRQ */
    if(d->config->cd_gpio != GPIO_NONE) {
        if(gpio_get_level(d->config->cd_gpio) != d->config->cd_active_level) {
            d->card_present = 0;
            d->card_present_last = 0;
        }

        system_set_irq_handler(GPIO_TO_IRQ(d->config->cd_gpio),
                               msc == 0 ? msc0_cd_interrupt : msc1_cd_interrupt);
        gpio_set_function(d->config->cd_gpio, GPIOF_IRQ_EDGE(1));
        gpio_flip_edge_irq(d->config->cd_gpio);
        gpio_enable_irq(d->config->cd_gpio);
    }
}

void msc_init(void)
{
    /* Only do this once -- each storage subsystem calls us in its init */
    static bool done = false;
    if(done)
        return;
    done = true;

    /* Set up each MSC driver according to msc_configs */
    for(int i = 0; i < MSC_COUNT; ++i)
        msc_init_one(&msc_drivers[i], i);
}

msc_drv* msc_get(int type, int index)
{
    for(int i = 0, m = 0; i < MSC_COUNT; ++i) {
        if(msc_drivers[i].config == NULL)
            continue;
        if(type == MSC_TYPE_ANY || msc_drivers[i].config->msc_type == type)
            if(index == m++)
                return &msc_drivers[i];
    }

    return NULL;
}

msc_drv* msc_get_by_drive(int drive_nr)
{
    for(int i = 0; i < MSC_COUNT; ++i)
        if(msc_drivers[i].drive_nr == drive_nr)
            return &msc_drivers[i];
    return NULL;
}

void msc_lock(msc_drv* d)
{
    mutex_lock(&d->lock);
}

void msc_unlock(msc_drv* d)
{
    mutex_unlock(&d->lock);
}

void msc_full_reset(msc_drv* d)
{
    msc_lock(d);
    msc_set_clock_mode(d, MSC_CLK_AUTOMATIC);
    msc_set_speed(d, MSC_SPEED_INIT);
    msc_set_width(d, 1);
    msc_ctl_reset(d);
    d->driver_flags = 0;
    memset(&d->cardinfo, 0, sizeof(tCardInfo));
    msc_unlock(d);
}

bool msc_card_detect(msc_drv* d)
{
    if(d->config->cd_gpio == GPIO_NONE)
        return true;

    return gpio_get_level(d->config->cd_gpio) == d->config->cd_active_level;
}

void msc_led_trigger(void)
{
    bool state = false;
    for(int i = 0; i < MSC_COUNT; ++i)
        if(msc_drivers[i].req_running)
            state = true;

    led(state);
}

/* ---------------------------------------------------------------------------
 * Controller API
 */

void msc_ctl_reset(msc_drv* d)
{
    /* Ingenic code suggests a reset changes clkrt */
    int clkrt = REG_MSC_CLKRT(d->msc_nr);

    /* Send reset -- bit is NOT self clearing */
    jz_overwritef(MSC_CTRL(d->msc_nr), RESET(1));
    udelay(100);
    jz_writef(MSC_CTRL(d->msc_nr), RESET(0));

    /* Verify reset in the status register */
    long deadline = current_tick + HZ;
    while(jz_readf(MSC_STAT(d->msc_nr), IS_RESETTING) &&
          current_tick < deadline) {
        sleep(1);
    }

    /* Ensure the clock state is as expected */
    if(d->clk_status & MSC_CLKST_AUTO)
        jz_writef(MSC_LPM(d->msc_nr), ENABLE(1));
    else if(d->clk_status & MSC_CLKST_ENABLE)
        jz_overwritef(MSC_CTRL(d->msc_nr), CLOCK_V(START));
    else
        jz_overwritef(MSC_CTRL(d->msc_nr), CLOCK_V(STOP));

    /* Clear and mask interrupts */
    REG_MSC_IMASK(d->msc_nr) = 0xffffffff;
    REG_MSC_IFLAG(d->msc_nr) = 0xffffffff;

    /* Restore clkrt */
    REG_MSC_CLKRT(d->msc_nr) = clkrt;
}

void msc_set_clock_mode(msc_drv* d, int mode)
{
    int cur_mode = (d->clk_status & MSC_CLKST_AUTO) ? MSC_CLK_AUTOMATIC
                                                    : MSC_CLK_MANUAL;
    if(mode == cur_mode)
        return;

    d->clk_status &= ~MSC_CLKST_ENABLE;
    if(mode == MSC_CLK_AUTOMATIC) {
        d->clk_status |= MSC_CLKST_AUTO;
        jz_writef(MSC_CTRL(d->msc_nr), CLOCK_V(STOP));
        jz_writef(MSC_LPM(d->msc_nr), ENABLE(1));
    } else {
        d->clk_status &= ~MSC_CLKST_AUTO;
        jz_writef(MSC_LPM(d->msc_nr), ENABLE(0));
        jz_writef(MSC_CTRL(d->msc_nr), CLOCK_V(STOP));
    }
}

void msc_enable_clock(msc_drv* d, bool enable)
{
    if(d->clk_status & MSC_CLKST_AUTO)
        return;

    bool is_enabled = (d->clk_status & MSC_CLKST_ENABLE);
    if(enable == is_enabled)
        return;

    if(enable) {
        jz_writef(MSC_CTRL(d->msc_nr), CLOCK_V(START));
        d->clk_status |= MSC_CLKST_ENABLE;
    } else {
        jz_writef(MSC_CTRL(d->msc_nr), CLOCK_V(STOP));
        d->clk_status &= ~MSC_CLKST_ENABLE;
    }
}

void msc_set_speed(msc_drv* d, int rate)
{
    /* Shut down clock while we change frequencies */
    if(d->clk_status & MSC_CLKST_ENABLE)
        jz_writef(MSC_CTRL(d->msc_nr), CLOCK_V(STOP));

    /* Wait for clock to go idle */
    while(jz_readf(MSC_STAT(d->msc_nr), CLOCK_EN))
        sleep(1);

    /* freq1 is output by MSCxDIV; freq2 is output by MSC_CLKRT */
    uint32_t freq1 = rate;
    uint32_t freq2 = rate;
    if(freq1 < MSC_SPEED_FAST)
        freq1 = MSC_SPEED_FAST;

    /* Handle MSCxDIV */
    uint32_t src_freq = clk_get(MSC_CLOCK_SOURCE) / 2;
    uint32_t div = clk_calc_div(src_freq, freq1);
    if(d->msc_nr == 0) {
        jz_writef(CPM_MSC0CDR, CE(1), CLKDIV(div - 1));
        while(jz_readf(CPM_MSC0CDR, BUSY));
        jz_writef(CPM_MSC0CDR, CE(0));
    } else {
        jz_writef(CPM_MSC1CDR, CE(1), CLKDIV(div - 1));
        while(jz_readf(CPM_MSC1CDR, BUSY));
        jz_writef(CPM_MSC1CDR, CE(0));
    }

    /* Handle MSC_CLKRT */
    uint32_t clkrt = clk_calc_shift(src_freq/div, freq2);
    REG_MSC_CLKRT(d->msc_nr) = clkrt;

    /* Handle frequency dependent timing settings
     * TODO - these settings might be SD specific...
     */
    uint32_t out_freq = (src_freq/div) >> clkrt;
    if(out_freq > MSC_SPEED_FAST) {
        jz_writef(MSC_LPM(d->msc_nr),
                  DRV_SEL_V(RISE_EDGE_DELAY_QTR_PHASE),
                  SMP_SEL_V(RISE_EDGE_DELAYED));
        jz_writef(MSC_CTRL2(d->msc_nr), SPEED_V(HIGHSPEED));
    } else {
        jz_writef(MSC_LPM(d->msc_nr),
                  DRV_SEL_V(FALL_EDGE),
                  SMP_SEL_V(RISE_EDGE));
        jz_writef(MSC_CTRL2(d->msc_nr), SPEED_V(DEFAULT));
    }

    /* Restart clock if it was running before */
    if(d->clk_status & MSC_CLKST_ENABLE)
        jz_writef(MSC_CTRL(d->msc_nr), CLOCK_V(START));
}

void msc_set_width(msc_drv* d, int width)
{
    /* Bus width is controlled per command with MSC_CMDAT. */
    if(width == 8)
        jz_vwritef(d->cmdat_def, MSC_CMDAT, BUS_WIDTH_V(8BIT));
    else if(width == 4)
        jz_vwritef(d->cmdat_def, MSC_CMDAT, BUS_WIDTH_V(4BIT));
    else
        jz_vwritef(d->cmdat_def, MSC_CMDAT, BUS_WIDTH_V(1BIT));
}

/* ---------------------------------------------------------------------------
 * Request API
 */

/* Note -- this must only be called with IRQs disabled */
static void msc_finish_request(msc_drv* d, int status)
{
    REG_MSC_IMASK(d->msc_nr) = 0xffffffff;
    REG_MSC_IFLAG(d->msc_nr) = 0xffffffff;
    if(d->req->flags & MSC_RF_DATA)
        jz_writef(MSC_DMAC(d->msc_nr), ENABLE(0));

    d->req->status = status;
    d->req_running = 0;
    d->iflag_done = 0;

    msc_led_trigger();
    timeout_cancel(&d->cmd_tmo);
    semaphore_release(&d->cmd_done);
}

static int msc_req_timeout(struct timeout* tmo)
{
    msc_drv* d = (msc_drv*)tmo->data;
    msc_async_abort(d, MSC_REQ_LOCKUP);
    return 0;
}

void msc_async_start(msc_drv* d, msc_req* r)
{
    /* Determined needed cmdat and interrupts */
    unsigned cmdat = d->cmdat_def;
    d->iflag_done = jz_orm(MSC_IFLAG, END_CMD_RES);

    cmdat |= jz_orf(MSC_CMDAT, RESP_FMT(r->resptype & ~MSC_RESP_BUSY));
    if(r->resptype & MSC_RESP_BUSY)
        cmdat |= jz_orm(MSC_CMDAT, BUSY);

    if(r->flags & MSC_RF_INIT)
        cmdat |= jz_orm(MSC_CMDAT, INIT);

    if(r->flags & MSC_RF_DATA) {
        cmdat |= jz_orm(MSC_CMDAT, DATA_EN);
        if(r->flags & MSC_RF_PROG)
            d->iflag_done = jz_orm(MSC_IFLAG, WR_ALL_DONE);
        else
            d->iflag_done = jz_orm(MSC_IFLAG, DMA_DATA_DONE);
    }

    if(r->flags & MSC_RF_WRITE)
        cmdat |= jz_orm(MSC_CMDAT, WRITE_READ);

    if(r->flags & MSC_RF_AUTO_CMD12)
        cmdat |= jz_orm(MSC_CMDAT, AUTO_CMD12);

    if(r->flags & MSC_RF_ABORT)
        cmdat |= jz_orm(MSC_CMDAT, IO_ABORT);

    unsigned imask = jz_orm(MSC_IMASK,
                            CRC_RES_ERROR, CRC_READ_ERROR, CRC_WRITE_ERROR,
                            TIME_OUT_RES, TIME_OUT_READ, END_CMD_RES);
    imask |= d->iflag_done;

    /* Program the controller */
    if(r->flags & MSC_RF_DATA) {
        REG_MSC_NOB(d->msc_nr) = r->nr_blocks;
        REG_MSC_BLKLEN(d->msc_nr) = r->block_len;
    }

    REG_MSC_CMD(d->msc_nr) = r->command;
    REG_MSC_ARG(d->msc_nr) = r->argument;
    REG_MSC_CMDAT(d->msc_nr) = cmdat;

    REG_MSC_IFLAG(d->msc_nr) = imask;
    REG_MSC_IMASK(d->msc_nr) &= ~imask;

    if(r->flags & MSC_RF_DATA) {
        d->dma_desc.nda = 0;
        d->dma_desc.mem = PHYSADDR(r->data);
        d->dma_desc.len = r->nr_blocks * r->block_len;
        d->dma_desc.cmd = 2; /* ID=0, ENDI=1, LINK=0 */
        commit_dcache_range(&d->dma_desc, sizeof(d->dma_desc));

        if(r->flags & MSC_RF_WRITE)
            commit_dcache_range(r->data, d->dma_desc.len);
        else
            discard_dcache_range(r->data, d->dma_desc.len);

        /* Unaligned address for DMA doesn't seem to work correctly.
         * FAT FS driver should ensure proper alignment of all buffers,
         * so in practice this panic should not occur, but if it does
         * I want to hear about it. */
        if(UNLIKELY(d->dma_desc.mem & 3)) {
            panicf("msc%d bad align: %08x", d->msc_nr,
                   (unsigned)d->dma_desc.mem);
        }

        jz_writef(MSC_DMAC(d->msc_nr), MODE_SEL(0), INCR(0), DMASEL(0));
        REG_MSC_DMANDA(d->msc_nr) = PHYSADDR(&d->dma_desc);
    }

    /* Begin processing */
    d->req = r;
    d->req_running = 1;
    msc_led_trigger();
    jz_writef(MSC_CTRL(d->msc_nr), START_OP(1));
    if(r->flags & MSC_RF_DATA)
        jz_writef(MSC_DMAC(d->msc_nr), ENABLE(1));

    /* TODO: calculate a suitable lower value for the lockup timeout.
     *
     * The SD spec defines timings based on the number of blocks transferred,
     * see sec. 4.6.2 "Read, write, and erase timeout conditions". This should
     * reduce the long delays which happen if errors occur.
     *
     * Also need to check if registers MSC_RDTO / MSC_RESTO are correctly set.
     */
    timeout_register(&d->cmd_tmo, msc_req_timeout, 10*HZ, (intptr_t)d);
}

void msc_async_abort(msc_drv* d, int status)
{
    int irq = disable_irq_save();
    if(d->req_running) {
        logf("msc%d: async abort status:%d", d->msc_nr, status);
        msc_finish_request(d, status);
    }

    restore_irq(irq);
}

int msc_async_wait(msc_drv* d, int timeout)
{
    if(semaphore_wait(&d->cmd_done, timeout) == OBJ_WAIT_TIMEDOUT)
        return MSC_REQ_INCOMPLETE;

    return d->req->status;
}

int msc_request(msc_drv* d, msc_req* r)
{
    msc_async_start(d, r);
    return msc_async_wait(d, TIMEOUT_BLOCK);
}

/* ---------------------------------------------------------------------------
 * Command response handling
 */

static void msc_read_response(msc_drv* d)
{
    unsigned res = REG_MSC_RES(d->msc_nr);
    unsigned dat;
    switch(d->req->resptype) {
    case MSC_RESP_R1:
    case MSC_RESP_R1B:
    case MSC_RESP_R3:
    case MSC_RESP_R6:
    case MSC_RESP_R7:
        dat = res << 24;
        res = REG_MSC_RES(d->msc_nr);
        dat |= res << 8;
        res = REG_MSC_RES(d->msc_nr);
        dat |= res & 0xff;
        d->req->response[0] = dat;
        break;

    case MSC_RESP_R2:
        for(int i = 0; i < 4; ++i) {
            dat = res << 24;
            res = REG_MSC_RES(d->msc_nr);
            dat |= res << 8;
            res = REG_MSC_RES(d->msc_nr);
            dat |= res >> 8;
            d->req->response[i] = dat;
        }

        break;

    default:
        return;
    }
}

static int msc_check_sd_response(msc_drv* d)
{
    if(d->req->resptype == MSC_RESP_R1 ||
       d->req->resptype == MSC_RESP_R1B) {
        if(d->req->response[0] & SD_R1_CARD_ERROR) {
            logf("msc%d: R1 card error: %08x", d->msc_nr, d->req->response[0]);
            return MSC_REQ_CARD_ERR;
        }
    }

    return MSC_REQ_SUCCESS;
}

static int msc_check_response(msc_drv* d)
{
    switch(d->config->msc_type) {
    case MSC_TYPE_SD:
        return msc_check_sd_response(d);
    default:
        /* TODO - implement msc_check_response for MMC and CE-ATA */
        return 0;
    }
}

/* ---------------------------------------------------------------------------
 * Interrupt handlers
 */

static void msc_interrupt(msc_drv* d)
{
    const unsigned tmo_bits = jz_orm(MSC_IFLAG, TIME_OUT_READ, TIME_OUT_RES);
    const unsigned crc_bits = jz_orm(MSC_IFLAG, CRC_RES_ERROR,
                                     CRC_READ_ERROR, CRC_WRITE_ERROR);
    const unsigned err_bits = tmo_bits | crc_bits;

    unsigned iflag = REG_MSC_IFLAG(d->msc_nr) & ~REG_MSC_IMASK(d->msc_nr);
    bool handled = false;

    /* In case card was removed */
    if(!msc_card_detect(d)) {
        msc_finish_request(d, MSC_REQ_EXTRACTED);
        return;
    }

    /* Check for errors */
    if(iflag & err_bits) {
        int st;
        if(iflag & crc_bits)
            st = MSC_REQ_CRC_ERR;
        else if(iflag & tmo_bits)
            st = MSC_REQ_TIMEOUT;
        else
            st = MSC_REQ_ERROR;

        msc_finish_request(d, st);
        return;
    }

    /* Read and check the command response */
    if(iflag & BM_MSC_IFLAG_END_CMD_RES) {
        msc_read_response(d);
        int st = msc_check_response(d);
        if(st == MSC_REQ_SUCCESS) {
            jz_writef(MSC_IMASK(d->msc_nr), END_CMD_RES(1));
            jz_overwritef(MSC_IFLAG(d->msc_nr), END_CMD_RES(1));
            handled = true;
        } else {
            msc_finish_request(d, st);
            return;
        }
    }

    /* Check if the "done" interrupt is signaled */
    if(iflag & d->iflag_done) {
        /* Discard after DMA in case of hardware cache prefetching.
         * Only needed for read operations.
         */
        if((d->req->flags & MSC_RF_DATA) != 0 &&
           (d->req->flags & MSC_RF_WRITE) == 0) {
            discard_dcache_range(d->req->data,
                                 d->req->block_len * d->req->nr_blocks);
        }

        msc_finish_request(d, MSC_REQ_SUCCESS);
        return;
    }

    if(!handled) {
        panicf("msc%d: irq bug! iflag:%08x raw_iflag:%08lx imask:%08lx",
               d->msc_nr, iflag, REG_MSC_IFLAG(d->msc_nr), REG_MSC_IMASK(d->msc_nr));
    }
}

static int msc_cd_callback(struct timeout* tmo)
{
    msc_drv* d = (msc_drv*)tmo->data;
    int now_present = msc_card_detect(d) ? 1 : 0;

    /* If the CD pin level changed during the timeout interval, then the
     * signal is not yet stable and we need to wait longer. */
    if(now_present != d->card_present_last) {
        d->card_present_last = now_present;
        return DEBOUNCE_TIME;
    }

    /* If there is a change, then broadcast the hotswap event */
    if(now_present != d->card_present) {
        if(now_present) {
            d->card_present = 1;
            queue_broadcast(SYS_HOTSWAP_INSERTED, d->drive_nr);
        } else {
            msc_async_abort(d, MSC_REQ_EXTRACTED);
            d->card_present = 0;
            queue_broadcast(SYS_HOTSWAP_EXTRACTED, d->drive_nr);
        }
    }

    return 0;
}

static void msc_cd_interrupt(msc_drv* d)
{
    /* Timer to debounce input */
    d->card_present_last = msc_card_detect(d) ? 1 : 0;
    timeout_register(&d->cd_tmo, msc_cd_callback, DEBOUNCE_TIME, (intptr_t)d);

    /* Invert the IRQ */
    gpio_flip_edge_irq(d->config->cd_gpio);
}

void MSC0(void)
{
    msc_interrupt(&msc_drivers[0]);
}

void MSC1(void)
{
    msc_interrupt(&msc_drivers[1]);
}

static void msc0_cd_interrupt(void)
{
    msc_cd_interrupt(&msc_drivers[0]);
}

static void msc1_cd_interrupt(void)
{
    msc_cd_interrupt(&msc_drivers[1]);
}

/* ---------------------------------------------------------------------------
 * SD command helpers
 */

int msc_cmd_exec(msc_drv* d, msc_req* r)
{
    int status = msc_request(d, r);
    if(status == MSC_REQ_SUCCESS)
        return status;
    else if(status == MSC_REQ_LOCKUP || status == MSC_REQ_EXTRACTED)
        d->driver_flags |= MSC_DF_ERRSTATE;
    else if(r->flags & (MSC_RF_ERR_CMD12|MSC_RF_AUTO_CMD12)) {
        /* After an error, the controller does not automatically issue CMD12,
         * so we need to send it if it's needed, as required by the SD spec.
         */
        msc_req nreq = {0};
        nreq.command = SD_STOP_TRANSMISSION;
        nreq.resptype = MSC_RESP_R1B;
        nreq.flags = MSC_RF_ABORT;
        logf("msc%d: cmd%d error, sending cmd12", d->msc_nr, r->command);
        if(msc_cmd_exec(d, &nreq))
            d->driver_flags |= MSC_DF_ERRSTATE;
    }

    logf("msc%d: err:%d, cmd%d, arg:%x", d->msc_nr, status,
         r->command, r->argument);
    return status;
}

int msc_app_cmd_exec(msc_drv* d, msc_req* r)
{
    msc_req areq = {0};
    areq.command = SD_APP_CMD;
    areq.argument = d->cardinfo.rca;
    areq.resptype = MSC_RESP_R1;
    if(msc_cmd_exec(d, &areq))
        return areq.status;

    /* Verify that CMD55 was accepted */
    if((areq.response[0] & (1 << 5)) == 0)
        return MSC_REQ_ERROR;

    return msc_cmd_exec(d, r);
}

int msc_cmd_go_idle_state(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_GO_IDLE_STATE;
    req.resptype = MSC_RESP_NONE;
    req.flags = MSC_RF_INIT;
    return msc_cmd_exec(d, &req);
}

int msc_cmd_send_if_cond(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_SEND_IF_COND;
    req.argument = 0x1aa;
    req.resptype = MSC_RESP_R7;

    /* TODO - Check if SEND_IF_COND timeout is really an error
     * IIRC, this can occur if the card isn't HCS (old cards < 2 GiB).
     */
    if(msc_cmd_exec(d, &req))
        return req.status;

    /* Set HCS bit if the card responds correctly */
    if((req.response[0] & 0xff) == 0xaa)
        d->driver_flags |= MSC_DF_HCS_CARD;

    return MSC_REQ_SUCCESS;
}

int msc_cmd_app_op_cond(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_APP_OP_COND;
    req.argument = 0x00300000; /* 3.4 - 3.6 V */
    req.resptype = MSC_RESP_R3;
    if(d->driver_flags & MSC_DF_HCS_CARD)
        req.argument |= (1 << 30);

    int timeout = 2 * HZ;
    do {
        if(msc_app_cmd_exec(d, &req))
            return req.status;
        if(req.response[0] & (1 << 31))
            break;
        sleep(1);
    } while(--timeout > 0);

    if(timeout == 0)
        return MSC_REQ_TIMEOUT;

    return MSC_REQ_SUCCESS;
}

int msc_cmd_all_send_cid(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_ALL_SEND_CID;
    req.resptype = MSC_RESP_R2;
    if(msc_cmd_exec(d, &req))
        return req.status;

    for(int i = 0; i < 4; ++i)
        d->cardinfo.cid[i] = req.response[i];

    return MSC_REQ_SUCCESS;
}

int msc_cmd_send_rca(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_SEND_RELATIVE_ADDR;
    req.resptype = MSC_RESP_R6;
    if(msc_cmd_exec(d, &req))
        return req.status;

    d->cardinfo.rca = req.response[0] & 0xffff0000;
    return MSC_REQ_SUCCESS;
}

int msc_cmd_send_csd(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_SEND_CSD;
    req.argument = d->cardinfo.rca;
    req.resptype = MSC_RESP_R2;
    if(msc_cmd_exec(d, &req))
        return req.status;

    for(int i = 0; i < 4; ++i)
        d->cardinfo.csd[i] = req.response[i];
    sd_parse_csd(&d->cardinfo);

    if((req.response[0] >> 30) == 1)
        d->driver_flags |= MSC_DF_V2_CARD;

    return 0;
}

int msc_cmd_select_card(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_SELECT_CARD;
    req.argument = d->cardinfo.rca;
    req.resptype = MSC_RESP_R1B;
    return msc_cmd_exec(d, &req);
}

int msc_cmd_set_bus_width(msc_drv* d, int width)
{
    /* TODO - must we check bus width is supported in the cardinfo? */
    msc_req req = {0};
    req.command = SD_SET_BUS_WIDTH;
    req.resptype = MSC_RESP_R1;
    switch(width) {
    case 1: req.argument = 0; break;
    case 4: req.argument = 2; break;
    default: return MSC_REQ_ERROR;
    }

    if(msc_app_cmd_exec(d, &req))
        return req.status;

    msc_set_width(d, width);
    return MSC_REQ_SUCCESS;
}

int msc_cmd_set_clr_card_detect(msc_drv* d, int arg)
{
    msc_req req = {0};
    req.command = SD_SET_CLR_CARD_DETECT;
    req.argument = arg;
    req.resptype = MSC_RESP_R1;
    return msc_app_cmd_exec(d, &req);
}

int msc_cmd_switch_freq(msc_drv* d)
{
    /* If card doesn't support High Speed, we don't need to send a command */
    if((d->driver_flags & MSC_DF_V2_CARD) == 0) {
        msc_set_speed(d, MSC_SPEED_FAST);
        return MSC_REQ_SUCCESS;
    }

    /* Try switching to High Speed (50 MHz) */
    char buffer[64] CACHEALIGN_ATTR;
    msc_req req = {0};
    req.command = SD_SWITCH_FUNC;
    req.argument = 0x80fffff1;
    req.resptype = MSC_RESP_R1;
    req.flags = MSC_RF_DATA;
    req.data = &buffer[0];
    req.block_len = 64;
    req.nr_blocks = 1;
    if(msc_cmd_exec(d, &req))
        return req.status;

    msc_set_speed(d, MSC_SPEED_HIGH);
    return MSC_REQ_SUCCESS;
}

int msc_cmd_send_status(msc_drv* d)
{
    msc_req req = {0};
    req.command = SD_SEND_STATUS;
    req.argument = d->cardinfo.rca;
    req.resptype = MSC_RESP_R1;
    return msc_cmd_exec(d, &req);
}

int msc_cmd_set_block_len(msc_drv* d, unsigned len)
{
    msc_req req = {0};
    req.command = SD_SET_BLOCKLEN;
    req.argument = len;
    req.resptype = MSC_RESP_R1;
    return msc_cmd_exec(d, &req);
}
