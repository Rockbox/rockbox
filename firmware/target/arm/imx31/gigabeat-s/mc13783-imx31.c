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
#include "spi-imx31.h"
#include "gpio-imx31.h"
#include "mc13783.h"
#include "debug.h"
#include "kernel.h"

#include "power-imx31.h"
#include "button-target.h"
#include "adc-target.h"
#include "usb-target.h"

#ifdef BOOTLOADER
#define PMIC_DRIVER_CLOSE
#endif

/* This is all based on communicating with the MC13783 PMU which is on 
 * CSPI2 with the chip select at 0. The LCD controller resides on
 * CSPI3 cs1, but we have no idea how to communicate to it */
static struct spi_node mc13783_spi =
{
    CSPI2_NUM,                     /* CSPI module 2 */
    CSPI_CONREG_CHIP_SELECT_SS0 |  /* Chip select 0 */
    CSPI_CONREG_DRCTL_DONT_CARE |  /* Don't care about CSPI_RDY */
    CSPI_CONREG_DATA_RATE_DIV_4 |  /* Clock = IPG_CLK/4 - 16.5MHz */
    CSPI_BITCOUNT(32-1) |          /* All 32 bits are to be transferred */
    CSPI_CONREG_SSPOL |            /* SS active high */
    CSPI_CONREG_SSCTL |            /* Negate SS between SPI bursts */
    CSPI_CONREG_MODE,              /* Master mode */
    0,                             /* SPI clock - no wait states */
};

extern const struct mc13783_event_list mc13783_event_list;

static int mc13783_thread_stack[DEFAULT_STACK_SIZE/sizeof(int)];
static const char *mc13783_thread_name = "pmic";
static struct wakeup mc13783_wake;

/* Tracking for which interrupts are enabled */
static uint32_t pmic_int_enabled[2] =
    { 0x00000000, 0x00000000 };

static const unsigned char pmic_intm_regs[2] =
    { MC13783_INTERRUPT_MASK0, MC13783_INTERRUPT_MASK1 };

static const unsigned char pmic_ints_regs[2] =
    { MC13783_INTERRUPT_STATUS0, MC13783_INTERRUPT_STATUS1 };

#ifdef PMIC_DRIVER_CLOSE
static bool pmic_close = false;
static struct thread_entry *mc13783_thread_p = NULL;
#endif

static void mc13783_interrupt_thread(void)
{
    uint32_t pending[2];

    /* Enable mc13783 GPIO event */
    gpio_enable_event(MC13783_EVENT_ID);

    while (1)
    {
        const struct mc13783_event *event, *event_last;

        wakeup_wait(&mc13783_wake, TIMEOUT_BLOCK);

#ifdef PMIC_DRIVER_CLOSE
        if (pmic_close)
            break;
#endif

        mc13783_read_regset(pmic_ints_regs, pending, 2);

        /* Only clear interrupts being dispatched */
        pending[0] &= pmic_int_enabled[0];
        pending[1] &= pmic_int_enabled[1];

        mc13783_write_regset(pmic_ints_regs, pending, 2);

        event = mc13783_event_list.events;
        event_last = event + mc13783_event_list.count;

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
                break; /* Teminate early if nothing more to service */
        }
        while (++event < event_last);
    }

#ifdef PMIC_DRIVER_CLOSE
    gpio_disable_event(MC13783_EVENT_ID);
#endif
}

/* GPIO interrupt handler for mc13783 */
void mc13783_event(void)
{
    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);
    wakeup_signal(&mc13783_wake);
}

void mc13783_init(void)
{
    /* Serial interface must have been initialized first! */
    wakeup_init(&mc13783_wake);

    /* Enable the PMIC SPI module */
    spi_enable_module(&mc13783_spi);

    /* Mask any PMIC interrupts for now - modules will enable them as
     * required */
    mc13783_write(MC13783_INTERRUPT_MASK0, 0xffffff);
    mc13783_write(MC13783_INTERRUPT_MASK1, 0xffffff);

    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);

#ifdef PMIC_DRIVER_CLOSE
    mc13783_thread_p =
#endif
        create_thread(mc13783_interrupt_thread,
            mc13783_thread_stack, sizeof(mc13783_thread_stack), 0,
            mc13783_thread_name IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}

#ifdef PMIC_DRIVER_CLOSE
void mc13783_close(void)
{
    struct thread_entry *thread = mc13783_thread_p;

    if (thread == NULL)
        return;

    mc13783_thread_p = NULL;

    pmic_close = true;
    wakeup_signal(&mc13783_wake);
    thread_wait(thread);
}
#endif /* PMIC_DRIVER_CLOSE */

bool mc13783_enable_event(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event =
        &mc13783_event_list.events[id];
    int set = event->set;
    uint32_t mask = event->mask;

    spi_lock(&mc13783_spi);

    pmic_int_enabled[set] |= mask;
    mc13783_clear(pmic_intm_regs[set], mask);

    spi_unlock(&mc13783_spi);

    return true;
}

void mc13783_disable_event(enum mc13783_event_ids id)
{
    const struct mc13783_event * const event =
        &mc13783_event_list.events[id];
    int set = event->set;
    uint32_t mask = event->mask;

    spi_lock(&mc13783_spi);

    pmic_int_enabled[set] &= ~mask;
    mc13783_set(pmic_intm_regs[set], mask);

    spi_unlock(&mc13783_spi);
}

uint32_t mc13783_set(unsigned address, uint32_t bits)
{
    spi_lock(&mc13783_spi);

    uint32_t data = mc13783_read(address);

    if (data != MC13783_DATA_ERROR)
        mc13783_write(address, data | bits);

    spi_unlock(&mc13783_spi);

    return data;
}

uint32_t mc13783_clear(unsigned address, uint32_t bits)
{
    spi_lock(&mc13783_spi);

    uint32_t data = mc13783_read(address);

    if (data != MC13783_DATA_ERROR)
        mc13783_write(address, data & ~bits);

    spi_unlock(&mc13783_spi);

    return data;
}

int mc13783_write(unsigned address, uint32_t data)
{
    struct spi_transfer xfer;
    uint32_t packet;

    if (address >= MC13783_NUM_REGS)
        return -1;

    packet = (1 << 31) | (address << 25) | (data & 0xffffff);
    xfer.txbuf = &packet;
    xfer.rxbuf = &packet;
    xfer.count = 1;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return -1;

    return 1 - xfer.count;
}

uint32_t mc13783_write_masked(unsigned address, uint32_t data, uint32_t mask)
{
    uint32_t old;

    spi_lock(&mc13783_spi);

    old = mc13783_read(address);

    if (old != MC13783_DATA_ERROR)
    {
        data = (old & ~mask) | (data & mask);

        if (mc13783_write(address, data) != 1)
            old = MC13783_DATA_ERROR;
    }

    spi_unlock(&mc13783_spi);

    return old;
}

int mc13783_write_regset(const unsigned char *regs, const uint32_t *data,
                         int count)
{
    int i;
    struct spi_transfer xfer;
    uint32_t packets[MC13783_NUM_REGS];

    if ((unsigned)count > MC13783_NUM_REGS)
        return -1;

    for (i = 0; i < count; i++)
    {
        uint32_t reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return -1;

        packets[i] = (1 << 31) | (reg << 25) | (data[i] & 0xffffff);
    }

    xfer.txbuf = packets;
    xfer.rxbuf = packets;
    xfer.count = count;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return -1;

    return count - xfer.count;
}

uint32_t mc13783_read(unsigned address)
{
    uint32_t packet;
    struct spi_transfer xfer;

    if (address >= MC13783_NUM_REGS)
        return MC13783_DATA_ERROR;

    packet = address << 25;

    xfer.txbuf = &packet;
    xfer.rxbuf = &packet;
    xfer.count = 1;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return MC13783_DATA_ERROR;

    return packet;
}

int mc13783_read_regset(const unsigned char *regs, uint32_t *buffer,
                        int count)
{
    int i;
    struct spi_transfer xfer;

    if ((unsigned)count > MC13783_NUM_REGS)
        return -1;

    for (i = 0; i < count; i++)
    {
        unsigned reg = regs[i];

        if (reg >= MC13783_NUM_REGS)
            return -1;

        buffer[i] = reg << 25;
    }

    xfer.txbuf = buffer;
    xfer.rxbuf = buffer;
    xfer.count = count;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return -1;

    return count - xfer.count;
}
