/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2007 Will Robertson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "spi-imx31.h"
#include "debug.h"
#include "kernel.h"

    /* This is all based on communicating with the MC13783 PMU which is on 
     * CSPI2 with the chip select at 0. The LCD controller resides on
     * CSPI3 cs1, but we have no idea how to communicate to it */

void spi_init(void) {
    CSPI_CONREG2 |= (2 << 20);      // Burst will be triggered at SPI_RDY low
    CSPI_CONREG2 |= (2 << 16);      // Clock = IPG_CLK/16  - we want about 20mhz
    CSPI_CONREG2 |= (31 << 8);      // All 32 bits are to be transferred
    CSPI_CONREG2 |= (1 << 3);       // Start burst on TXFIFO write.
    CSPI_CONREG2 |= (1 << 1);       // Master mode.
    CSPI_CONREG2 |= 1;              // Enable CSPI2;
}

static int spi_transfer(int address, long data, long* buffer, bool read) {
    return -1; /* Disable for now - hangs - and we'll use interrupts */

    unsigned long packet = 0;
    if(!read) {
        /* Set the appropriate bit in the packet to indicate a write */
        packet |= (1<<31);
    }
    /* Set the address of the packet */
    packet |= (address << 25);

    /* Ensure data only occupies 24 bits, then mash the data into the packet */
    data &= ~(DATAMASK);
    packet |= data;

    /* Wait for some room in TXFIFO */
    while(CSPI_STATREG2 & (1<<2));

    /* Send the packet */
    CSPI_TXDATA2 = packet;

    /* Poll the XCH bit to wait for the end of the transfer, with
     * a one second timeout */
    int newtick = current_tick + HZ;
    while((CSPI_CONREG2 & (1<<2)) && (current_tick < newtick));

    if(newtick > current_tick) {
        *buffer = CSPI_RXDATA2;
        return 0;
    } else {
        /* Indicate the fact that the transfer timed out */
        return -1;
    }
}

void spi_send(int address, unsigned long data) {
    long dummy;
    if(spi_transfer(address, data, &dummy, false)) {
        DEBUGF("SPI Send timed out");
    }
}

void spi_read(int address, unsigned long* buffer) {
    if(spi_transfer(address, 0, buffer, true)) {
        DEBUGF("SPI read timed out");
    }
}
