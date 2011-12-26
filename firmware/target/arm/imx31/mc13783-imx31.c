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

extern const struct mc13783_event mc13783_events[MC13783_NUM_EVENTS];
extern struct spi_node mc13783_spi;

/* PMIC event service data */
static int mc13783_thread_stack[DEFAULT_STACK_SIZE/sizeof(int)];
static const char * const mc13783_thread_name = "pmic";
static struct semaphore mc13783_svc_wake;

/* Tracking for which interrupts are enabled */
static uint32_t pmic_int_enabled[2] =
    { 0x00000000, 0x00000000 };

static const unsigned char pmic_intm_regs[2] =
    { MC13783_INTERRUPT_MASK0, MC13783_INTERRUPT_MASK1 };

static const unsigned char pmic_ints_regs[2] =
    { MC13783_INTERRUPT_STATUS0, MC13783_INTERRUPT_STATUS1 };

static volatile unsigned int mc13783_thread_id = 0;

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

static void mc13783_interrupt_thread(void)
{
    uint32_t pending[2];

    /* Enable mc13783 GPIO event */
    gpio_enable_event(MC13783_EVENT_ID);

    while (1)
    {
        const struct mc13783_event *event, *event_last;

        semaphore_wait(&mc13783_svc_wake, TIMEOUT_BLOCK);

        if (mc13783_thread_id == 0)
            break;

        mc13783_read_regs(pmic_ints_regs, pending, 2);

        /* Only clear interrupts being dispatched */
        pending[0] &= pmic_int_enabled[0];
        pending[1] &= pmic_int_enabled[1];

        mc13783_write_regs(pmic_ints_regs, pending, 2);

        /* Whatever is going to be serviced in this loop has been
         * acknowledged. Reenable interrupt and if anything was still
         * pending or became pending again, another signal will be
         * generated. */
        bitset32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);

        event = mc13783_events;
        event_last = event + MC13783_NUM_EVENTS;

        /* .count is surely expected to be > 0 */
        do
        {
            unsigned int set = event->int_id / MC13783_INT_ID_SET_DIV;
            uint32_t pnd = pending[set];
            uint32_t mask = 1 << (event->int_id & MC13783_INT_ID_NUM_MASK);

            if (pnd & mask)
            {
                event->callback();
                pending[set] = pnd & ~mask;
            }

            if ((pending[0] | pending[1]) == 0)
                break; /* Terminate early if nothing more to service */
        }
        while (++event < event_last);
    }

    gpio_disable_event(MC13783_EVENT_ID);
}

/* GPIO interrupt handler for mc13783 */
void mc13783_event(void)
{
    /* Mask the interrupt (unmasked when PMIC thread services it). */
    bitclr32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);
    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);
    semaphore_release(&mc13783_svc_wake);
}

void INIT_ATTR mc13783_init(void)
{
    /* Serial interface must have been initialized first! */
    semaphore_init(&mc13783_svc_wake, 1, 0);

    /* Enable the PMIC SPI module */
    spi_enable_module(&mc13783_spi, true);

    /* Mask any PMIC interrupts for now - modules will enable them as
     * required */
    mc13783_write(MC13783_INTERRUPT_MASK0, 0xffffff);
    mc13783_write(MC13783_INTERRUPT_MASK1, 0xffffff);

    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);

    mc13783_thread_id =
        create_thread(mc13783_interrupt_thread,
            mc13783_thread_stack, sizeof(mc13783_thread_stack), 0,
            mc13783_thread_name IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}

void mc13783_close(void)
{
    unsigned int thread_id = mc13783_thread_id;

    if (thread_id == 0)
        return;

    mc13783_thread_id = 0;
    semaphore_release(&mc13783_svc_wake);
    thread_wait(thread_id);
    spi_enable_module(&mc13783_spi, false);
}

bool mc13783_enable_event(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event = &mc13783_events[id];
    unsigned int set = event->int_id / MC13783_INT_ID_SET_DIV;
    uint32_t mask = 1 << (event->int_id & MC13783_INT_ID_NUM_MASK);

    pmic_int_enabled[set] |= mask;
    mc13783_clear(pmic_intm_regs[set], mask);

    return true;
}

void mc13783_disable_event(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event = &mc13783_events[id];
    unsigned int set = event->int_id / MC13783_INT_ID_SET_DIV;
    uint32_t mask = 1 << (event->int_id & MC13783_INT_ID_NUM_MASK);

    pmic_int_enabled[set] &= ~mask;
    mc13783_set(pmic_intm_regs[set], mask);
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
