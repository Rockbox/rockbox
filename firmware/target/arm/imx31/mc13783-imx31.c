/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 by Michael Sevakis
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
#include "cpu.h"
#define DEFINE_MC13783_VECTOR_TABLE
#include "mc13783-target.h"
#include "gpio-target.h"
#include "debug.h"
#include "kernel.h"

/* Extend the basic SPI transfer descriptor with our own fields */
struct mc13783_transfer_desc
{
    struct spi_transfer_desc xfer;
    union
    {
        /* Pick _either_ data or semaphore */
        struct semaphore sema;
        uint32_t data[4];
    };
};

static uint32_t pmic_int_enb[2];                  /* Enabled ints */
static uint32_t pmic_int_sense_enb[2];            /* Enabled sense reading */
static struct mc13783_transfer_desc int_xfers[2]; /* ISR transfer descriptor */
static const struct mc13783_event *current_event; /* Current event in callback */
static bool int_restore;                          /* Prevent SPI callback from
                                                     unmasking GPIO interrupt
                                                     (lockout) */

static const struct mc13783_event * event_from_id(enum mc13783_int_ids id)
{
    for (unsigned int i = 0; i < mc13783_event_vector_tbl_len; i++)
    {
        if (mc13783_event_vector_tbl[i].id == id)
            return &mc13783_event_vector_tbl[i];
    }

    return NULL;
}

/* Called when a transfer is finished and data is ready/written */
static void mc13783_xfer_complete_cb(struct spi_transfer_desc *xfer)
{
    semaphore_release(&((struct mc13783_transfer_desc *)xfer)->sema);
}

static inline bool wait_for_transfer_complete(struct mc13783_transfer_desc *xfer)
{
    return semaphore_wait(&xfer->sema, TIMEOUT_BLOCK)
            == OBJ_WAIT_SUCCEEDED && xfer->xfer.count == 0;
}

static inline bool mc13783_transfer(struct spi_transfer_desc *xfer,
                                    uint32_t *txbuf,
                                    uint32_t *rxbuf,
                                    int count,
                                    spi_transfer_cb_fn_type callback)
{
    xfer->node = &mc13783_spi;
    xfer->txbuf = txbuf;
    xfer->rxbuf = rxbuf;
    xfer->count = count;
    xfer->callback = callback;
    xfer->next = NULL;

    return spi_transfer(xfer);
}

static inline void sync_transfer_init(struct mc13783_transfer_desc *xfer)
{
    semaphore_init(&xfer->sema, 1, 0);
}

static inline bool mc13783_sync_transfer(struct mc13783_transfer_desc *xfer,
                                         uint32_t *txbuf,
                                         uint32_t *rxbuf,
                                         int count)
{
    sync_transfer_init(xfer);
    return mc13783_transfer(&xfer->xfer, txbuf, rxbuf, count, mc13783_xfer_complete_cb);
}

/* Efficient interrupt status and acking */
static void mc13783_int_svc_complete_callback(struct spi_transfer_desc *xfer)
{
    struct mc13783_transfer_desc *desc1 = (struct mc13783_transfer_desc *)xfer;
    uint32_t pnd0 = desc1->data[0], pnd1 = desc1->data[1];

    /* Restore PMIC interrupt events */
    if (int_restore)
        gpio_int_enable(MC13783_EVENT_ID);

    /* Call handlers */
    const struct mc13783_event *event = mc13783_event_vector_tbl;
    while (pnd0 | pnd1)
    {
        uint32_t id = event->id;
        uint32_t set = id / 32;
        uint32_t bit = 1 << (id % 32);

        uint32_t pnd = set == 0 ? pnd0 : pnd1;
        if (pnd & bit)
        {
            current_event = event;
            event->callback();
            set == 0 ? (pnd0 &= ~bit) : (pnd1 &= ~bit);
        }

        event++;
    }
}

static void mc13783_int_svc_callback(struct spi_transfer_desc *xfer)
{
    /* Only clear interrupts with handlers */
    struct mc13783_transfer_desc *desc0 = (struct mc13783_transfer_desc *)xfer;
    struct mc13783_transfer_desc *desc1 = &int_xfers[1];

    uint32_t pnd0 = desc0->data[0] & pmic_int_enb[0];
    uint32_t pnd1 = desc0->data[1] & pmic_int_enb[1];

    desc1->data[0] = pnd0;
    desc1->data[1] = pnd1;

    /* Setup the write packets with status(es) to clear */
    desc0->data[0] = (1 << 31) | (MC13783_INTERRUPT_STATUS0 << 25) | pnd0;
    desc0->data[1] = (1 << 31) | (MC13783_INTERRUPT_STATUS1 << 25) | pnd1;

    /* Only read sense if any pending interrupts have them enabled */
    if ((pnd0 & pmic_int_sense_enb[0]) || (pnd1 & pmic_int_sense_enb[1]))
    {
        desc0->data[2] = MC13783_INTERRUPT_SENSE0 << 25;
        desc0->data[3] = MC13783_INTERRUPT_SENSE1 << 25;
        desc1->xfer.rxbuf = desc0->data;
        desc1->xfer.count = 4;
    }
}

/* GPIO interrupt handler for mc13783 */
void INT_MC13783(void)
{
    /* Mask the interrupt (unmasked after final read services it). */
    gpio_int_disable(MC13783_EVENT_ID);
    gpio_int_clear(MC13783_EVENT_ID);

    /* Setup the read packets */
    int_xfers[0].data[0] = MC13783_INTERRUPT_STATUS0 << 25;
    int_xfers[0].data[1] = MC13783_INTERRUPT_STATUS1 << 25;

    unsigned long cpsr = disable_irq_save();

    /* Do these without intervening transfers */
    if (mc13783_transfer(&int_xfers[0].xfer, int_xfers[0].data,
                         int_xfers[0].data, 2, mc13783_int_svc_callback))
    {
        /* Start this provisionally and fill-in actual values during the
           first transfer's callback - set whatever could be known */
        mc13783_transfer(&int_xfers[1].xfer, int_xfers[0].data, NULL, 2,
                         mc13783_int_svc_complete_callback);
    }

    restore_irq(cpsr);
}

void INIT_ATTR mc13783_init(void)
{
    /* Serial interface must have been initialized first! */

    /* Enable the PMIC SPI module */
    spi_enable_node(&mc13783_spi, true);

    /* Mask any PMIC interrupts for now - modules will enable them as
     * required */
    mc13783_write(MC13783_INTERRUPT_MASK0, 0xffffff);
    mc13783_write(MC13783_INTERRUPT_MASK1, 0xffffff);

    gpio_int_clear(MC13783_EVENT_ID);
    gpio_enable_event(MC13783_EVENT_ID, true);
}

void mc13783_close(void)
{
    int_restore = false;
    gpio_int_disable(MC13783_EVENT_ID);
    gpio_enable_event(MC13783_EVENT_ID, false);
    spi_enable_node(&mc13783_spi, false);
}

void mc13783_enable_event(enum mc13783_int_ids id, bool enable)
{
    static const unsigned char pmic_intm_regs[2] =
        { MC13783_INTERRUPT_MASK0, MC13783_INTERRUPT_MASK1 };

    const struct mc13783_event * const event = event_from_id(id);
    if (event == NULL)
        return;

    unsigned int set = id / 32;
    uint32_t mask = 1 << (id % 32);
    uint32_t bit = enable ? mask : 0;

    /* Mask GPIO while changing bits around */
    int_restore = false;
    gpio_int_disable(MC13783_EVENT_ID);
    mc13783_write_masked(pmic_intm_regs[set], bit ^ mask, mask);
    bitmod32(&pmic_int_sense_enb[set], event->sense ? bit : 0, mask);
    bitmod32(&pmic_int_enb[set], bit, mask);
    int_restore = true;
    gpio_int_enable(MC13783_EVENT_ID);
}

uint32_t mc13783_event_sense(void)
{
    const struct mc13783_event *event = current_event;
    return int_xfers[0].data[2 + event->id / 32] & event->sense;
}

uint32_t mc13783_set(unsigned address, uint32_t bits)
{
    return mc13783_write_masked(address, bits, bits);
}

uint32_t mc13783_clear(unsigned address, uint32_t bits)
{
    return mc13783_write_masked(address, 0, bits);
}

/* Called when the first transfer of mc13783_write_masked is complete */
static void mc13783_write_masked_cb(struct spi_transfer_desc *xfer)
{
    struct mc13783_transfer_desc *desc = (struct mc13783_transfer_desc *)xfer;
    desc->data[1] |= desc->data[0] & desc->data[2]; /* & ~mask */
}

uint32_t mc13783_write_masked(unsigned address, uint32_t data, uint32_t mask)
{
    if (address >= MC13783_NUM_REGS)
        return MC13783_DATA_ERROR;

    mask &= 0xffffff;

    struct mc13783_transfer_desc xfers[2];
    xfers[0].data[0] = address << 25;
    xfers[0].data[1] = (1 << 31) | (address << 25) | (data & mask);
    xfers[0].data[2] = ~mask;

    unsigned long cpsr = disable_irq_save();

    /* Queue up two transfers in a row */
    bool ok = mc13783_transfer(&xfers[0].xfer,
                               &xfers[0].data[0], &xfers[0].data[0], 1,
                               mc13783_write_masked_cb) &&
              mc13783_sync_transfer(&xfers[1], &xfers[0].data[1], NULL, 1);

    restore_irq(cpsr);

    if (ok && wait_for_transfer_complete(&xfers[1]))
        return xfers[0].data[0];

    return MC13783_DATA_ERROR;
}

uint32_t mc13783_read(unsigned address)
{
    if (address >= MC13783_NUM_REGS)
        return MC13783_DATA_ERROR;

    uint32_t packet = address << 25;

    struct mc13783_transfer_desc xfer;
    if (mc13783_sync_transfer(&xfer, &packet, &packet, 1) &&
        wait_for_transfer_complete(&xfer))
    {
        return packet;
    }

    return MC13783_DATA_ERROR;
}

int mc13783_write(unsigned address, uint32_t data)
{
    if (address >= MC13783_NUM_REGS)
        return -1;

    uint32_t packet = (1 << 31) | (address << 25) | (data & 0xffffff);

    struct mc13783_transfer_desc xfer;
    if (mc13783_sync_transfer(&xfer, &packet, NULL, 1) &&
        wait_for_transfer_complete(&xfer))
    {
        return 1 - xfer.xfer.count;
    }

    return -1;
}

int mc13783_read_regs(const unsigned char *regs, uint32_t *buffer,
                      int count)
{
    struct mc13783_transfer_desc xfer;
    sync_transfer_init(&xfer);

    if (mc13783_read_async(&xfer.xfer, regs, buffer, count,
                           mc13783_xfer_complete_cb) &&
        wait_for_transfer_complete(&xfer))
    {
        return count - xfer.xfer.count;
    }

    return -1;
}

int mc13783_write_regs(const unsigned char *regs, uint32_t *buffer,
                       int count)
{
    struct mc13783_transfer_desc xfer;
    sync_transfer_init(&xfer);

    if (mc13783_write_async(&xfer.xfer, regs, buffer, count,
                            mc13783_xfer_complete_cb) &&
        wait_for_transfer_complete(&xfer))
    {
        return count - xfer.xfer.count;
    }

    return -1;
}

bool mc13783_read_async(struct spi_transfer_desc *xfer,
                        const unsigned char *regs, uint32_t *buffer,
                        int count, spi_transfer_cb_fn_type callback)
{
    for (int i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return false;

        buffer[i] = reg << 25;
    }

    return mc13783_transfer(xfer, buffer, buffer, count, callback);
}

bool mc13783_write_async(struct spi_transfer_desc *xfer,
                         const unsigned char *regs, uint32_t *buffer,
                         int count, spi_transfer_cb_fn_type callback)
{
    for (int i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return false;

        buffer[i] = (1 << 31) | (reg << 25) | (buffer[i] & 0xffffff);
    }

    return mc13783_transfer(xfer, buffer, NULL, count, callback);
}
