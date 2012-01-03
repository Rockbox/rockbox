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
#include "gpio-imx31.h"
#include "mc13783.h"
#include "mc13783-target.h"
#include "debug.h"
#include "kernel.h"

/* Extend the basic SPI transfer descriptor with our own fields */
struct mc13783_transfer_desc
{
    struct spi_transfer_desc xfer;
    union
    {
        struct semaphore sema;
        uint32_t data;
    };
};

extern const struct mc13783_event mc13783_events[MC13783_NUM_EVENTS];
extern struct spi_node mc13783_spi;

static uint32_t pmic_int_enb[2]; /* Enabled ints */
static uint32_t pmic_int_sense_enb[2]; /* Enabled sense reading */
static uint32_t int_pnd_buf[2];  /* Pending ints */
static uint32_t int_data_buf[4]; /* ISR data buffer */
static struct spi_transfer_desc int_xfers[2]; /* ISR transfer descriptor */
static bool restore_event = true; /* Protect SPI callback from unmasking GPIO
                                     interrupt (lockout) */

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

/* Efficient interrupt status and acking */
static void mc13783_int_svc_complete_callback(struct spi_transfer_desc *xfer)
{
    /* Restore PMIC interrupt events */
    if (restore_event)
        bitset32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);

    /* Call handlers */
    for (
        const struct mc13783_event *event = mc13783_events;
        int_pnd_buf[0] | int_pnd_buf[1];
        event++
    )
    {
        unsigned int set = event->int_id / MC13783_INT_ID_SET_DIV;
        uint32_t pnd = int_pnd_buf[set];
        uint32_t mask = 1 << (event->int_id & MC13783_INT_ID_NUM_MASK);

        if (pnd & mask)
        {
            event->callback();
            int_pnd_buf[set] = pnd & ~mask;
        }
    }

    (void)xfer;
}

static void mc13783_int_svc_callback(struct spi_transfer_desc *xfer)
{
    /* Only clear interrupts with handlers */
    int_pnd_buf[0] &= pmic_int_enb[0];
    int_pnd_buf[1] &= pmic_int_enb[1];

    /* Only read sense if enabled interrupts have them enabled */
    if ((int_pnd_buf[0] & pmic_int_sense_enb[0]) ||
        (int_pnd_buf[1] & pmic_int_sense_enb[1]))
    {
        int_data_buf[2] = MC13783_INTERRUPT_SENSE0 << 25;
        int_data_buf[3] = MC13783_INTERRUPT_SENSE1 << 25;
        int_xfers[1].rxbuf = int_data_buf;
        int_xfers[1].count = 4;
    }

    /* Setup the write packets with status(es) to clear */
    int_data_buf[0] = (1 << 31) | (MC13783_INTERRUPT_STATUS0 << 25)
                        | int_pnd_buf[0];
    int_data_buf[1] = (1 << 31) | (MC13783_INTERRUPT_STATUS1 << 25)
                        | int_pnd_buf[1];
    (void)xfer;
}

/* GPIO interrupt handler for mc13783 */
void mc13783_event(void)
{
    /* Mask the interrupt (unmasked after final read services it). */
    bitclr32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);
    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);

    /* Setup the read packets */
    int_pnd_buf[0] = MC13783_INTERRUPT_STATUS0 << 25;
    int_pnd_buf[1] = MC13783_INTERRUPT_STATUS1 << 25;

    unsigned long cpsr = disable_irq_save();

    /* Do these without intervening transfers */
    if (mc13783_transfer(&int_xfers[0], int_pnd_buf, int_pnd_buf, 2,
                         mc13783_int_svc_callback))
    {
        /* Start this provisionally and fill-in actual values during the
           first transfer's callback - set whatever could be known */
        mc13783_transfer(&int_xfers[1], int_data_buf, NULL, 2,
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

    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);
    gpio_enable_event(MC13783_EVENT_ID);
}

void mc13783_close(void)
{
    restore_event = false;
    gpio_disable_event(MC13783_EVENT_ID);
    spi_enable_node(&mc13783_spi, false);
}

void mc13783_enable_event(enum mc13783_event_ids id, bool enable)
{
    static const unsigned char pmic_intm_regs[2] =
        { MC13783_INTERRUPT_MASK0, MC13783_INTERRUPT_MASK1 };

    const struct mc13783_event * const event = &mc13783_events[id];
    unsigned int set = event->int_id / MC13783_INT_ID_SET_DIV;
    uint32_t mask = 1 << (event->int_id & MC13783_INT_ID_NUM_MASK);

    /* Mask GPIO while changing bits around */
    restore_event = false;
    bitclr32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);
    mc13783_write_masked(pmic_intm_regs[set],
                         enable ? 0 : mask, mask);
    bitmod32(&pmic_int_enb[set], enable ? mask : 0, mask);
    bitmod32(&pmic_int_sense_enb[set], enable ? event->sense : 0,
             event->sense);
    restore_event = true;
    bitset32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);
}

uint32_t mc13783_event_sense(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event = &mc13783_events[id];
    unsigned int set = event->int_id / MC13783_INT_ID_SET_DIV;
    return int_data_buf[2 + set] & event->sense;
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
    uint32_t *packets = desc->xfer.rxbuf; /* Will have been advanced by 1 */
    packets[0] |= packets[-1] & ~desc->data;
}

uint32_t mc13783_write_masked(unsigned address, uint32_t data, uint32_t mask)
{
    if (address >= MC13783_NUM_REGS)
        return MC13783_DATA_ERROR;

    mask &= 0xffffff;

    uint32_t packets[2] =
    {
        address << 25,
        (1 << 31) | (address << 25) | (data & mask)
    };

    struct mc13783_transfer_desc xfers[2];
    xfers[0].data = mask;
    semaphore_init(&xfers[1].sema, 1, 0);

    unsigned long cpsr = disable_irq_save();

    /* Queue up two transfers in a row */
    bool ok = mc13783_transfer(&xfers[0].xfer, &packets[0], &packets[0], 1,
                               mc13783_write_masked_cb) &&
              mc13783_transfer(&xfers[1].xfer, &packets[1], NULL, 1,
                               mc13783_xfer_complete_cb);

    restore_irq(cpsr);

    if (ok && wait_for_transfer_complete(&xfers[1]))
        return packets[0];

    return MC13783_DATA_ERROR;
}

uint32_t mc13783_read(unsigned address)
{
    if (address >= MC13783_NUM_REGS)
        return MC13783_DATA_ERROR;

    uint32_t packet = address << 25;

    struct mc13783_transfer_desc xfer;
    semaphore_init(&xfer.sema, 1, 0);

    if (mc13783_transfer(&xfer.xfer, &packet, &packet, 1,
                         mc13783_xfer_complete_cb) &&
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
    semaphore_init(&xfer.sema, 1, 0);

    if (mc13783_transfer(&xfer.xfer, &packet, NULL, 1,
                         mc13783_xfer_complete_cb) &&
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
    semaphore_init(&xfer.sema, 1, 0);

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
    semaphore_init(&xfer.sema, 1, 0);

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
