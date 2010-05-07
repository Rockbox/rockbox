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
static const char *mc13783_thread_name = "pmic";
static struct wakeup mc13783_svc_wake;

/* Synchronous thread communication objects */
static struct mutex mc13783_spi_mutex;
static struct wakeup mc13783_spi_wake;

/* Tracking for which interrupts are enabled */
static uint32_t pmic_int_enabled[2] =
    { 0x00000000, 0x00000000 };

static const unsigned char pmic_intm_regs[2] =
    { MC13783_INTERRUPT_MASK0, MC13783_INTERRUPT_MASK1 };

static const unsigned char pmic_ints_regs[2] =
    { MC13783_INTERRUPT_STATUS0, MC13783_INTERRUPT_STATUS1 };

static volatile unsigned int mc13783_thread_id = 0;

static void mc13783_xfer_complete_cb(struct spi_transfer_desc *trans);

/* Transfer descriptor for synchronous reads and writes */
static struct spi_transfer_desc mc13783_transfer =
{
    .node = &mc13783_spi,
    .txbuf = NULL,
    .rxbuf = NULL,
    .count = 0,
    .callback = mc13783_xfer_complete_cb,
    .next = NULL,
};

/* Called when a transfer is finished and data is ready/written */
static void mc13783_xfer_complete_cb(struct spi_transfer_desc *xfer)
{
    if (xfer->count != 0)
        return;

    wakeup_signal(&mc13783_spi_wake);
}

static inline bool wait_for_transfer_complete(void)
{
    return wakeup_wait(&mc13783_spi_wake, HZ*2) == OBJ_WAIT_SUCCEEDED &&
           mc13783_transfer.count == 0;
}

static void mc13783_interrupt_thread(void)
{
    uint32_t pending[2];

    /* Enable mc13783 GPIO event */
    gpio_enable_event(MC13783_EVENT_ID);

    while (1)
    {
        const struct mc13783_event *event, *event_last;

        wakeup_wait(&mc13783_svc_wake, TIMEOUT_BLOCK);

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
        imx31_regset32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);

        event = mc13783_events;
        event_last = event + MC13783_NUM_EVENTS;

        /* .count is surely expected to be > 0 */
        do
        {
            enum mc13783_event_sets set = event->set;
            uint32_t pnd = pending[set];
            uint32_t mask = event->mask;

            if (pnd & mask)
            {
                event->callback();
                pnd &= ~mask;
                pending[set] = pnd;
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
    imx31_regclr32(&MC13783_GPIO_IMR, 1ul << MC13783_GPIO_LINE);
    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);
    wakeup_signal(&mc13783_svc_wake);
}

void mc13783_init(void)
{
    /* Serial interface must have been initialized first! */
    wakeup_init(&mc13783_svc_wake);
    mutex_init(&mc13783_spi_mutex);

    wakeup_init(&mc13783_spi_wake);

    /* Enable the PMIC SPI module */
    spi_enable_module(&mc13783_spi);

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
    wakeup_signal(&mc13783_svc_wake);
    thread_wait(thread_id);
    spi_disable_module(&mc13783_spi);
}

bool mc13783_enable_event(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event = &mc13783_events[id];
    int set = event->set;
    uint32_t mask = event->mask;

    mutex_lock(&mc13783_spi_mutex);

    pmic_int_enabled[set] |= mask;
    mc13783_clear(pmic_intm_regs[set], mask);

    mutex_unlock(&mc13783_spi_mutex);

    return true;
}

void mc13783_disable_event(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event = &mc13783_events[id];
    int set = event->set;
    uint32_t mask = event->mask;

    mutex_lock(&mc13783_spi_mutex);

    pmic_int_enabled[set] &= ~mask;
    mc13783_set(pmic_intm_regs[set], mask);

    mutex_unlock(&mc13783_spi_mutex);
}

uint32_t mc13783_set(unsigned address, uint32_t bits)
{
    uint32_t data;

    mutex_lock(&mc13783_spi_mutex);

    data = mc13783_read(address);

    if (data != MC13783_DATA_ERROR)
        mc13783_write(address, data | bits);

    mutex_unlock(&mc13783_spi_mutex);

    return data;
}

uint32_t mc13783_clear(unsigned address, uint32_t bits)
{
    uint32_t data;

    mutex_lock(&mc13783_spi_mutex);

    data = mc13783_read(address);

    if (data != MC13783_DATA_ERROR)
        mc13783_write(address, data & ~bits);

    mutex_unlock(&mc13783_spi_mutex);

    return data;
}

int mc13783_write(unsigned address, uint32_t data)
{
    uint32_t packet;
    int i;

    if (address >= MC13783_NUM_REGS)
        return -1;

    packet = (1 << 31) | (address << 25) | (data & 0xffffff);

    mutex_lock(&mc13783_spi_mutex);

    mc13783_transfer.txbuf = &packet;
    mc13783_transfer.rxbuf = NULL;
    mc13783_transfer.count = 1;

    i = -1;

    if (spi_transfer(&mc13783_transfer) && wait_for_transfer_complete())
        i = 1 - mc13783_transfer.count;

    mutex_unlock(&mc13783_spi_mutex);

    return i;
}

uint32_t mc13783_write_masked(unsigned address, uint32_t data, uint32_t mask)
{
    uint32_t old;

    mutex_lock(&mc13783_spi_mutex);

    old = mc13783_read(address);

    if (old != MC13783_DATA_ERROR)
    {
        data = (old & ~mask) | (data & mask);

        if (mc13783_write(address, data) != 1)
            old = MC13783_DATA_ERROR;
    }

    mutex_unlock(&mc13783_spi_mutex);

    return old;
}

uint32_t mc13783_read(unsigned address)
{
    uint32_t packet;

    if (address >= MC13783_NUM_REGS)
        return MC13783_DATA_ERROR;

    packet = address << 25;

    mutex_lock(&mc13783_spi_mutex);

    mc13783_transfer.txbuf = &packet;
    mc13783_transfer.rxbuf = &packet;
    mc13783_transfer.count = 1;

    if (!spi_transfer(&mc13783_transfer) || !wait_for_transfer_complete())
        packet = MC13783_DATA_ERROR;

    mutex_unlock(&mc13783_spi_mutex);

    return packet;
}

int mc13783_read_regs(const unsigned char *regs, uint32_t *buffer,
                      int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return -1;

        buffer[i] = reg << 25;
    }

    mutex_lock(&mc13783_spi_mutex);

    mc13783_transfer.txbuf = buffer;
    mc13783_transfer.rxbuf = buffer;
    mc13783_transfer.count = count;

    i = -1;

    if (spi_transfer(&mc13783_transfer) && wait_for_transfer_complete())
        i = count - mc13783_transfer.count;

    mutex_unlock(&mc13783_spi_mutex);

    return i;
}

int mc13783_write_regs(const unsigned char *regs, uint32_t *buffer,
                       int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return -1;

        buffer[i] = (1 << 31) | (reg << 25) | (buffer[i] & 0xffffff);
    }

    mutex_lock(&mc13783_spi_mutex);

    mc13783_transfer.txbuf = buffer;
    mc13783_transfer.rxbuf = NULL;
    mc13783_transfer.count = count;

    i = -1;

    if (spi_transfer(&mc13783_transfer) && wait_for_transfer_complete())
        i = count - mc13783_transfer.count;

    mutex_unlock(&mc13783_spi_mutex);

    return i;
}

#if 0 /* Not needed right now */
bool mc13783_read_async(struct spi_transfer_desc *xfer,
                        const unsigned char *regs, uint32_t *buffer,
                        int count, spi_transfer_cb_fn_type callback)
{
    int i;

    for (i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return false;

        buffer[i] = reg << 25;
    }

    xfer->node  = &mc13783_spi;
    xfer->txbuf = buffer;
    xfer->rxbuf = buffer;
    xfer->count = count;
    xfer->callback = callback;

    return spi_transfer(xfer);
}
#endif

bool mc13783_write_async(struct spi_transfer_desc *xfer,
                         const unsigned char *regs, uint32_t *buffer,
                         int count, spi_transfer_cb_fn_type callback)
{
    int i;

    for (i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return false;

        buffer[i] = (1 << 31) | (reg << 25) | (buffer[i] & 0xffffff);
    }

    xfer->node  = &mc13783_spi;
    xfer->txbuf = buffer;
    xfer->rxbuf = NULL;
    xfer->count = count;
    xfer->callback = callback;

    return spi_transfer(xfer);
}
