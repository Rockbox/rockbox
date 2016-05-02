/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "dma-imx233.h"
#include "i2c-imx233.h"
#include "pinctrl-imx233.h"
#include "string.h"

#include "regs/i2c.h"

/**
 * Driver Architecture:
 * The driver has two interfaces: the good'n'old i2c_* api and a more
 * advanced one specific to the imx233 dma architecture. The i2c_* api is
 * implemented with the imx233_i2c_* one.
 * Since each i2c transfer must be split into several dma transfers and we
 * cannot do dynamic allocation, we allow for at most I2C_NR_STAGES stages.
 * A typical read memory transfer will require 3 stages thus 4 is safe:
 * - one with start, device address and memory address
 * - one with repeated start and device address
 * - one with data read and stop
 * To make the interface easier to use and to handle the DMA/cache related
 * issues, all the data transfers are done in a statically allocated buffer
 * which is managed by the driver. The driver will ensure that all transfers
 * are cache aligned and will copy back the data to user buffers at the end.
 * The I2C_BUFFER_SIZE define controls the size of the buffer. All transfers
 * should probably fit within 512 bytes.
 *
 * On top of this, transfers are queued using the 'next' field of imx233_i2c_xfer_t.
 * Each time a transfer is programmed, it is translated to dma transfers using
 * the dma API.
 */

/**
 * Internal DMA API to build the transfer.
 * NOTE the api does not perform any locking, it is up to the caller to ensure
 * that there only one transfer beint built at any time.
 */

/* start building a transfer */
static void imx233_i2c_begin(void);
/* add a stage
 * NOTE for transmit, the data is copied to a buffer so the buffer can be freed
 * afer this function return. For receive, buffer must exists until transfer is
 * complete. This function assumes any receive transfer is final (master will
 * send NAK). */
static void imx233_i2c_add(bool start, bool transmit, void *buffer, unsigned size, bool stop);
/* end building a transfer and start the transfer */
static void imx233_i2c_kick(void);
/* abort transfer (will call imx233_i2c_transfer_complete) */
static void imx233_i2c_abort(void);
/* set speed */
static void imx233_i2c_set_speed(bool fast_mode);
/* callback function when transfer is finished */
static void imx233_i2c_transfer_complete(enum imx233_i2c_error_t status);

/**
 * Advanced API
 */

/* NOTE these variables are not marked as volatile because all functions
 * do all operation with IRQ disabled, so they won't change their value
 * in the middle of a function */
static struct imx233_i2c_xfer_t *i2c_head; /* pointer to the current transfer */
static struct imx233_i2c_xfer_t *i2c_tail; /* pointer to the last transfer */
static struct timeout i2c_tmo; /* timeout */

/* timeout callback */
static int imx233_i2c_timeout(struct timeout *tmo);

/* called in IRQ context or with IRQ disabled */
static void imx233_i2c_start(void)
{
    uint8_t addr_wr = i2c_head->dev_addr;
    uint8_t addr_rd = i2c_head->dev_addr | 1;
    /* translate transfer using DMA API */
    imx233_i2c_set_speed(i2c_head->fast_mode);
    imx233_i2c_begin();
    if(i2c_head->mode == I2C_WRITE)
    {
        /* START + addr */
        imx233_i2c_add(true, true, &addr_wr, 1, false);
        /* data + stop if no second stage */
        imx233_i2c_add(false, true, i2c_head->data[0], i2c_head->count[0], i2c_head->count[1] == 0);
        /* (if second stage) data + stop */
        if(i2c_head->count[1] > 0)
            imx233_i2c_add(false, true, i2c_head->data[1], i2c_head->count[1], true);
    }
    else /* I2C_READ */
    {
        /* (if write stage) */
        if(i2c_head->count[0] > 0)
        {
            /* START + addr */
            imx233_i2c_add(true, true, &addr_wr, 1, false);
            /* data */
            imx233_i2c_add(false, true, i2c_head->data[0], i2c_head->count[0], false);
        }
        /* START + addr */
        imx233_i2c_add(true, true, &addr_rd, 1, false);
        /* read data + stop */
        imx233_i2c_add(false, false, i2c_head->data[1], i2c_head->count[1], true);
    }
    /* kick transfer */
    imx233_i2c_kick();
    /* setup timer for timeout */
    if(i2c_head->tmo_ms > 0)
        timeout_register(&i2c_tmo, imx233_i2c_timeout, i2c_head->tmo_ms * HZ / 1000, 0);
}

/* unqueue head and notify completion, called with IRQ disabled */
static void imx233_i2c_unqueue_head(enum imx233_i2c_error_t status)
{
    /* notify */
    if(i2c_head->callback)
        i2c_head->callback(i2c_head, status);
    /* unqueue */
    i2c_head = i2c_head->next;
}

/* callback function when transfer is finished, called with IRQ disabled */
static void imx233_i2c_transfer_complete(enum imx233_i2c_error_t status)
{
    /* cancel timeout
     * NOTE because IRQ are disabled, the timeout callback will not be called
     * until we enable them back, at which point we will have disabled the timeout
     * so the completion routine will not be called twice. */
    if(i2c_head->tmo_ms > 0)
        timeout_cancel(&i2c_tmo);
    /* notify completion and unqueue
     * WARNING completion callback can queue other transfers, so the only part
     * of the queue that cannot change is this transaction, everything else can
     * change */
    struct imx233_i2c_xfer_t *this_xfer = i2c_head;
    struct imx233_i2c_xfer_t *last_xfer = i2c_head->last; /* in transaction */
    /* unqueue head */
    imx233_i2c_unqueue_head(status);
    /* in case of failure, skip others */
    if(status != I2C_SUCCESS && this_xfer != last_xfer)
        while(i2c_head != last_xfer)
                imx233_i2c_unqueue_head(I2C_SKIP);
    /* if there is anything left, start it */
    if(i2c_head)
        imx233_i2c_start();
}

static int imx233_i2c_timeout(struct timeout *tmo)
{
    (void) tmo;
    imx233_i2c_abort();
    return 0; /* do not fire again */
}

void imx233_i2c_transfer(struct imx233_i2c_xfer_t *xfer)
{
    /* avoid any race with the irq handler */
    unsigned long cpsr = disable_irq_save();
    /* before queuing, update link to last transfer in each transfer */
    struct imx233_i2c_xfer_t *last = xfer;
    while(last->next)
        last = last->next;
    struct imx233_i2c_xfer_t *tmp = xfer;
    while(tmp)
    {
        tmp->last = last;
        tmp = tmp->next;
    }
    /* no transfer pending: start one */
    if(i2c_head == NULL)
    {
        i2c_head = xfer;
        i2c_tail = last;
        /* kick transfer now */
        imx233_i2c_start();
    }
    /* pending transer: queue and let the irq handler process it for us */
    else
    {
        i2c_tail->next = xfer;
        i2c_tail = last;
    }

    restore_irq(cpsr);
}

/**
 * DMA API implementation
 */

/* Used for DMA */
struct i2c_dma_command_t
{
    struct apb_dma_command_t dma;
    /* PIO words */
    uint32_t ctrl0;
    /* copy buffer pointers */
    void *src;
    void *dst;
    /* padded to next multiple of cache line size (32 bytes) */
    uint32_t pad[2];
} __attribute__((packed)) CACHEALIGN_ATTR;

__ENSURE_STRUCT_CACHE_FRIENDLY(struct i2c_dma_command_t)

#define I2C_NR_STAGES   4
#define I2C_BUFFER_SIZE 512
/* Current transfer */
static int i2c_nr_stages;
static struct i2c_dma_command_t i2c_stage[I2C_NR_STAGES];
static uint8_t i2c_buffer[I2C_BUFFER_SIZE] CACHEALIGN_ATTR;
static uint32_t i2c_buffer_end; /* current end */

static void imx233_i2c_reset(void)
{
    /* clear softreset */
    imx233_reset_block(&HW_I2C_CTRL0);
    /* Errata (imx233):
     * When RETAIN_CLOCK is set, the ninth clock pulse (ACK) is not generated. However, the SDA
     * line is read at the proper timing interval. If RETAIN_CLOCK is cleared, the ninth clock pulse is
     * generated.
     * HW_I2C_CTRL1[ACK_MODE] has default value of 0. It should be set to 1 to enable the fix for
     * this issue.
     */
#if IMX233_SUBTARGET >= 3780
    BF_SET(I2C_CTRL1, ACK_MODE);
#endif
    BF_SET(I2C_CTRL0, CLKGATE);
}

void imx233_i2c_init(void)
{
    BF_SET(I2C_CTRL0, SFTRST);
    /* setup pins (must be done when shutdown) */
    imx233_pinctrl_setup_vpin(VPIN_I2C_SCL, "i2c scl", PINCTRL_DRIVE_4mA, true);
    imx233_pinctrl_setup_vpin(VPIN_I2C_SDA, "i2c sda", PINCTRL_DRIVE_4mA, true);
    imx233_i2c_reset();
    i2c_head = i2c_tail = NULL;
}

static void imx233_i2c_begin(void)
{
    /* wakeup */
    BF_CLR(I2C_CTRL0, CLKGATE);
    i2c_nr_stages = 0;
    i2c_buffer_end = 0;
}

static void imx233_i2c_add(bool start, bool transmit,
    void *buffer, unsigned size, bool stop)
{
    if(i2c_nr_stages == I2C_NR_STAGES)
        panicf("i2c: too many stages");
    /* align buffer end on cache boundary */
    uint32_t start_off = CACHEALIGN_UP(i2c_buffer_end);
    uint32_t end_off = start_off + size;
    if(end_off > I2C_BUFFER_SIZE)
        panicf("i2c: transfer is too big");
    i2c_buffer_end = end_off;
    if(transmit)
    {
        /* copy data to buffer */
        memcpy(i2c_buffer + start_off, buffer, size);
    }
    else
    {
        /* record pointers for finalization */
        i2c_stage[i2c_nr_stages].src = i2c_buffer + start_off;
        i2c_stage[i2c_nr_stages].dst = buffer;
    }

    if(i2c_nr_stages > 0)
    {
        i2c_stage[i2c_nr_stages - 1].dma.next = &i2c_stage[i2c_nr_stages].dma;
        i2c_stage[i2c_nr_stages - 1].dma.cmd |= BM_APB_CHx_CMD_CHAIN;
        if(!start)
            i2c_stage[i2c_nr_stages - 1].ctrl0 |= BM_I2C_CTRL0_RETAIN_CLOCK;
    }
    i2c_stage[i2c_nr_stages].dma.buffer = i2c_buffer + start_off;
    i2c_stage[i2c_nr_stages].dma.next = NULL;
    i2c_stage[i2c_nr_stages].dma.cmd = BF_OR(APB_CHx_CMD,
        COMMAND(transmit ? BV_APB_CHx_CMD_COMMAND__READ : BV_APB_CHx_CMD_COMMAND__WRITE),
        WAIT4ENDCMD(1), CMDWORDS(1), XFER_COUNT(size));
    /* assume that any read is final (send nak on last) */
    i2c_stage[i2c_nr_stages].ctrl0 = BF_OR(I2C_CTRL0,
        XFER_COUNT(size), DIRECTION(transmit), SEND_NAK_ON_LAST(!transmit),
        PRE_SEND_START(start), POST_SEND_STOP(stop), MASTER_MODE(1));
    i2c_nr_stages++;
}

static enum imx233_i2c_error_t imx233_i2c_finalize(void)
{
    discard_dcache_range(i2c_buffer, I2C_BUFFER_SIZE);
    for(int i = 0; i < i2c_nr_stages; i++)
    {
        struct i2c_dma_command_t *c = &i2c_stage[i];
        if(BF_RDX(c->dma.cmd, APB_CHx_CMD, COMMAND) == BV_APB_CHx_CMD_COMMAND__WRITE)
            memcpy(c->dst, c->src, BF_RDX(c->dma.cmd, APB_CHx_CMD, XFER_COUNT));
    }
    return I2C_SUCCESS;
}

static void imx233_i2c_kick(void)
{
    if(i2c_nr_stages == 0)
        panicf("i2c: empty kick");
    i2c_stage[i2c_nr_stages - 1].dma.cmd |= BM_APB_CHx_CMD_SEMAPHORE | BM_APB_CHx_CMD_IRQONCMPLT;

    BF_CLR(I2C_CTRL1, SLAVE_IRQ, SLAVE_STOP_IRQ, MASTER_LOSS_IRQ, EARLY_TERM_IRQ,
        OVERSIZE_XFER_TERM_IRQ, NO_SLAVE_ACK_IRQ, DATA_ENGINE_CMPLT_IRQ, BUS_FREE_IRQ);
    imx233_dma_reset_channel(APB_I2C);
    imx233_icoll_enable_interrupt(INT_SRC_I2C_DMA, true);
    imx233_icoll_enable_interrupt(INT_SRC_I2C_ERROR, true);
    imx233_dma_enable_channel_interrupt(APB_I2C, true);
    imx233_dma_start_command(APB_I2C, &i2c_stage[0].dma);
}

static void imx233_i2c_abort(void)
{
    /* FIXME there is a race condition here: if dma irq fires right before we
     * reset the channel, it will most likely trigger an IRQ anyway. It is
     * extremely unlikely but ideally, we should check this in the IRQ handler
     * with an id/counter. */
    imx233_dma_reset_channel(APB_I2C);
    imx233_i2c_reset();
    imx233_i2c_transfer_complete(I2C_TIMEOUT);
}

static enum imx233_i2c_error_t imx233_i2c_end(void)
{
    enum imx233_i2c_error_t ret;
    /* check for various errors */
    if(BF_RD(I2C_CTRL1, MASTER_LOSS_IRQ))
        ret = I2C_MASTER_LOSS;
    else if(BF_RD(I2C_CTRL1, NO_SLAVE_ACK_IRQ))
    {
        /* the core doesn't like this error, this is a workaround to prevent lock up */
#if IMX233_SUBTARGET >= 3780
        BF_SET(I2C_CTRL1, CLR_GOT_A_NAK);
#endif
        imx233_dma_reset_channel(APB_I2C);
        imx233_i2c_reset();
        ret = I2C_NO_SLAVE_ACK;
    }
    else if(BF_RD(I2C_CTRL1, EARLY_TERM_IRQ))
        ret = I2C_SLAVE_NAK;
    else
        ret = imx233_i2c_finalize();
    /* sleep */
    BF_SET(I2C_CTRL0, CLKGATE);
    return ret;
}

static void imx233_i2c_set_speed(bool fast_mode)
{
    /* See I2C specification for standard- and fast-mode timings
     * Clock is derived APBX which we assume to be running at 24 MHz. */
    if(fast_mode)
    {
        /* Fast-mode @ 400 kHz */
        HW_I2C_TIMING0 = 0x000f0007; /* HIGH_COUNT=0.6us, RCV_COUNT=0.2us */
        HW_I2C_TIMING1 = 0x001f000f; /* LOW_COUNT=1.3us, XMIT_COUNT=0.6us */
        HW_I2C_TIMING2 = 0x0015000d; /* BUS_FREE=0.9us LEADIN_COUNT=0.55us */
    }
    else
    {
        /* Standard-mode @ 100 kHz */
        HW_I2C_TIMING0 = 0x00780030; /* HIGH_COUNT=5us, RCV_COUNT=2us */
        HW_I2C_TIMING1 = 0x00800030; /* LOW_COUNT=5.3us, XMIT_COUNT=2us */
        HW_I2C_TIMING2 = 0x00300030; /* BUS_FREE=2us LEADIN_COUNT=2us */
    }
}

static void imx233_i2c_irq(bool err)
{
    if(err)
        panicf("i2c: dma error");
    /* reset dma channel on error */
    if(imx233_dma_is_channel_error_irq(APB_I2C))
        imx233_dma_reset_channel(APB_I2C);
    /* clear irq flags */
    imx233_dma_clear_channel_interrupt(APB_I2C);
    /* handle completion */
    imx233_i2c_transfer_complete(imx233_i2c_end());
}

void INT_I2C_DMA(void)
{
    imx233_i2c_irq(false);
}

void INT_I2C_ERROR(void)
{
    imx233_i2c_irq(true);
}

/** Public API */

void i2c_init(void)
{
}

struct imx233_i2c_sync_xfer_t
{
    struct imx233_i2c_xfer_t xfer;
    struct semaphore sema;
    volatile enum imx233_i2c_error_t status;
};

/* synchronous callback: record status and release semaphore */
static void i2c_sync_callback(struct imx233_i2c_xfer_t *xfer, enum imx233_i2c_error_t status)
{
    struct imx233_i2c_sync_xfer_t *sxfer = (void *)xfer;
    sxfer->status = status;
    semaphore_release(&sxfer->sema);
}

static int i2c_sync_transfer(struct imx233_i2c_sync_xfer_t *xfer)
{
    semaphore_init(&xfer->sema, 1, 0);
    /* common init */
    xfer->xfer.next = NULL;
    xfer->xfer.callback = &i2c_sync_callback;
    xfer->xfer.fast_mode = true;
    xfer->xfer.tmo_ms = 1000;
    /* kick */
    imx233_i2c_transfer(&xfer->xfer);
    /* wait */
    semaphore_wait(&xfer->sema, TIMEOUT_BLOCK);
    return (int)xfer->status;
}

int i2c_write(int device, const unsigned char* buf, int count)
{
    struct imx233_i2c_sync_xfer_t xfer;
    xfer.xfer.dev_addr = device;
    xfer.xfer.mode = I2C_WRITE;
    xfer.xfer.count[0] = count;
    xfer.xfer.data[0] = (void *)buf;
    xfer.xfer.count[1] = 0;
    return i2c_sync_transfer(&xfer);
}

int i2c_read(int device, unsigned char* buf, int count)
{
    struct imx233_i2c_sync_xfer_t xfer;
    xfer.xfer.dev_addr = device;
    xfer.xfer.mode = I2C_READ;
    xfer.xfer.count[0] = 0;
    xfer.xfer.count[1] = count;
    xfer.xfer.data[1] = buf;
    return i2c_sync_transfer(&xfer);
}

int i2c_readmem(int device, int address, unsigned char* buf, int count)
{
    uint8_t addr = address; /* assume 1 byte */
    struct imx233_i2c_sync_xfer_t xfer;
    xfer.xfer.dev_addr = device;
    xfer.xfer.mode = I2C_READ;
    xfer.xfer.count[0] = 1;
    xfer.xfer.data[0] = &addr;
    xfer.xfer.count[1] = count;
    xfer.xfer.data[1] = buf;
    return i2c_sync_transfer(&xfer);
}

int i2c_writemem(int device, int address, const unsigned char* buf, int count)
{
    uint8_t addr = address; /* assume 1 byte */
    struct imx233_i2c_sync_xfer_t xfer;
    xfer.xfer.dev_addr = device;
    xfer.xfer.mode = I2C_WRITE;
    xfer.xfer.count[0] = 1;
    xfer.xfer.data[0] = &addr;
    xfer.xfer.count[1] = count;
    xfer.xfer.data[1] = (void *)buf;
    return i2c_sync_transfer(&xfer);
}
