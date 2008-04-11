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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "cpu.h"
#include "spi-imx31.h"
#include "mc13783.h"
#include "debug.h"
#include "kernel.h"

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

static int mc13783_thread_stack[DEFAULT_STACK_SIZE/sizeof(int)];
static const char *mc13783_thread_name = "pmic";
static struct wakeup mc13783_wake;

static __attribute__((noreturn)) void mc13783_interrupt_thread(void)
{
    while (1)
    {
        wakeup_wait(&mc13783_wake, TIMEOUT_BLOCK);
    }
}

static __attribute__((interrupt("IRQ"))) void mc13783_interrupt(void)
{
    wakeup_signal(&mc13783_wake);
}

void mc13783_init(void)
{
    /* Serial interface must have been initialized first! */
    wakeup_init(&mc13783_wake);

    /* Enable the PMIC SPI module */
    spi_enable_module(&mc13783_spi);

    create_thread(mc13783_interrupt_thread, mc13783_thread_stack,
                  sizeof(mc13783_thread_stack), 0, mc13783_thread_name
                  IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}

void mc13783_set(unsigned address, uint32_t bits)
{
    spi_lock(&mc13783_spi);
    uint32_t data = mc13783_read(address);
    mc13783_write(address, data | bits);
    spi_unlock(&mc13783_spi);
}

void mc13783_clear(unsigned address, uint32_t bits)
{
    spi_lock(&mc13783_spi);
    uint32_t data = mc13783_read(address);
    mc13783_write(address, data & ~bits);
    spi_unlock(&mc13783_spi);
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

int mc13783_write_multiple(unsigned start, const uint32_t *data, int count)
{
    int i;
    struct spi_transfer xfer;
    uint32_t packets[64];

    if (start + count > MC13783_NUM_REGS)
        return -1;

    /* Prepare payload */
    for (i = 0; i < count; i++, start++)
    {
        packets[i] = (1 << 31) | (start << 25) | (data[i] & 0xffffff);
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
        return (uint32_t)-1;

    packet = address << 25;

    xfer.txbuf = &packet;
    xfer.rxbuf = &packet;
    xfer.count = 1;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return (uint32_t)-1;

    return packet & 0xffffff;
}

int mc13783_read_multiple(unsigned start, uint32_t *buffer, int count)
{
    int i;
    uint32_t packets[64];
    struct spi_transfer xfer;

    if (start + count > MC13783_NUM_REGS)
        return -1;

    xfer.txbuf = packets;
    xfer.rxbuf = buffer;
    xfer.count = count;

    /* Prepare TX payload */
    for (i = 0; i < count; i++, start++)
        packets[i] = start << 25;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return -1;

    return count - xfer.count;
}
