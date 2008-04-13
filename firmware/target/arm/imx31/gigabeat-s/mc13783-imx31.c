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
#include "gpio-imx31.h"
#include "mc13783.h"
#include "debug.h"
#include "kernel.h"

#include "power-imx31.h"
#include "button-target.h"
#include "adc-target.h"

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

/* The next two functions are rather target-specific but they'll just be left
 * here for the moment */
static __attribute__((noreturn)) void mc13783_interrupt_thread(void)
{
    const unsigned char status_regs[2] =
    {
        MC13783_INTERRUPT_STATUS0,
        MC13783_INTERRUPT_STATUS1,
    };
    uint32_t pending[2];
    uint32_t value;

    mc13783_read_regset(status_regs, pending, 2);
    mc13783_write_regset(status_regs, pending, 2);

    gpio_enable_event(MC13783_GPIO_NUM, MC13783_EVENT_ID);

    /* Check initial states for events with a sense bit */
    value = mc13783_read(MC13783_INTERRUPT_SENSE0);
    set_charger_inserted(value & MC13783_CHGDET);

    value = mc13783_read(MC13783_INTERRUPT_SENSE1);
    button_power_set_state((value & MC13783_ONOFD1) == 0);
    set_headphones_inserted((value & MC13783_ONOFD2) == 0);

    pending[0] = pending[1] = 0xffffff;
    mc13783_write_regset(status_regs, pending, 2);

    /* Enable desired PMIC interrupts - some are unmasked in the drivers that
     * handle a specific task */
    mc13783_clear(MC13783_INTERRUPT_MASK0, MC13783_CHGDET);
    mc13783_clear(MC13783_INTERRUPT_MASK1, MC13783_ONOFD1 | MC13783_ONOFD2);
    
    while (1)
    {
        wakeup_wait(&mc13783_wake, TIMEOUT_BLOCK);

        mc13783_read_regset(status_regs, pending, 2);
        mc13783_write_regset(status_regs, pending, 2);

        if (pending[0])
        {
            /* Handle ...PENDING0 */

            /* Handle interrupts without a sense bit */
            if (pending[0] & MC13783_ADCDONE)
                adc_done();

            /* Handle interrupts that have a sense bit that needs to
             * be checked */
            if (pending[0] & MC13783_CHGDET)
            {
                value = mc13783_read(MC13783_INTERRUPT_SENSE0);

                if (pending[0] & MC13783_CHGDET)
                    set_charger_inserted(value & MC13783_CHGDET);
            }
        }

        if (pending[1])
        {
            /* Handle ...PENDING1 */

            /* Handle interrupts without a sense bit */
            /* ... */

            /* Handle interrupts that have a sense bit that needs to
             * be checked */
            if (pending[1] & (MC13783_ONOFD1 | MC13783_ONOFD2))
            {
                value = mc13783_read(MC13783_INTERRUPT_SENSE1);

                if (pending[1] & MC13783_ONOFD1)
                    button_power_set_state((value & MC13783_ONOFD1) == 0);

                if (pending[1] & MC13783_ONOFD2)
                    set_headphones_inserted((value & MC13783_ONOFD2) == 0);
            }
        }
    }
}

/* GPIO interrupt handler for mc13783 */
int mc13783_event(void)
{
    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);
    wakeup_signal(&mc13783_wake);
    return 1; /* Yes, it's handled */
}

void mc13783_init(void)
{
    /* Serial interface must have been initialized first! */
    wakeup_init(&mc13783_wake);

    /* Enable the PMIC SPI module */
    spi_enable_module(&mc13783_spi);

    /* Mask any PMIC interrupts for now - poll initial status in thread
     * and enable them there */
    mc13783_write(MC13783_INTERRUPT_MASK0, 0xffffff);
    mc13783_write(MC13783_INTERRUPT_MASK1, 0xffffff);

    MC13783_GPIO_ISR = (1ul << MC13783_GPIO_LINE);

    create_thread(mc13783_interrupt_thread, mc13783_thread_stack,
                  sizeof(mc13783_thread_stack), 0, mc13783_thread_name
                  IF_PRIO(, PRIORITY_REALTIME) IF_COP(, CPU));
}

uint32_t mc13783_set(unsigned address, uint32_t bits)
{
    spi_lock(&mc13783_spi);

    uint32_t data = mc13783_read(address);

    if (data != (uint32_t)-1)
        mc13783_write(address, data | bits);

    spi_unlock(&mc13783_spi);

    return data;
}

uint32_t mc13783_clear(unsigned address, uint32_t bits)
{
    spi_lock(&mc13783_spi);

    uint32_t data = mc13783_read(address);

    if (data != (uint32_t)-1)
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

int mc13783_write_multiple(unsigned start, const uint32_t *data, int count)
{
    int i;
    struct spi_transfer xfer;
    uint32_t packets[MC13783_NUM_REGS];

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

int mc13783_write_regset(const unsigned char *regs, const uint32_t *data,
                         int count)
{
    int i;
    struct spi_transfer xfer;
    uint32_t packets[MC13783_NUM_REGS];

    if (count > MC13783_NUM_REGS)
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
        return (uint32_t)-1;

    packet = address << 25;

    xfer.txbuf = &packet;
    xfer.rxbuf = &packet;
    xfer.count = 1;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return (uint32_t)-1;

    return packet;
}

int mc13783_read_multiple(unsigned start, uint32_t *buffer, int count)
{
    int i;
    struct spi_transfer xfer;

    if (start + count > MC13783_NUM_REGS)
        return -1;

    xfer.txbuf = buffer;
    xfer.rxbuf = buffer;
    xfer.count = count;

    /* Prepare TX payload */
    for (i = 0; i < count; i++, start++)
        buffer[i] = start << 25;

    if (!spi_transfer(&mc13783_spi, &xfer))
        return -1;

    return count - xfer.count;
}

int mc13783_read_regset(const unsigned char *regs, uint32_t *buffer,
                        int count)
{
    int i;
    struct spi_transfer xfer;

    if (count > MC13783_NUM_REGS)
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
