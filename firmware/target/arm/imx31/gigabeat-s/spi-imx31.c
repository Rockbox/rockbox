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

void spi_init(void) {
    CSPI_CONREG2 |= (2 << 20);      // Burst will be triggered at SPI_RDY low
    CSPI_CONREG2 |= (2 << 16);      // Clock = IPG_CLK/16  - we want about 20mhz
    CSPI_CONREG2 |= (31 << 8);      // All 32 bits are to be transferred
    CSPI_CONREG2 |= (1 << 3);       // Start burst on TXFIFO write.
    CSPI_CONREG2 |= (1 << 1);       // Master mode.
    CSPI_CONREG2 |= 1;              // Enable CSPI2;
}

void spi_send(int address, unsigned long data) {
    /* This is all based on communicating with the MC13783 PMU which is on 
     * CSPI2 with the chip select at 0. The LCD controller resides on
     * CSPI3 cs1, but we have no idea how to communicate to it */
    unsigned long packet = 0;
    packet |= (1<<31);              // This is a write
    packet |= (address << 25);
    data &= ~(DATAMASK);            // Ensure that data is only 24 bits
    packet |= data;

    while(CSPI_STATREG2 & (1<<2));  // Don't write while TXFIFO is full
    CSPI_TXDATA2 = packet;
}

void spi_read(int address, unsigned long* buffer) {
    unsigned long packet = 0;
    packet |= (address << 25);
    
    while(CSPI_STATREG2 & (1<<2));  // Wait till TXFIFO is empty
    CSPI_TXDATA2 = packet;

    short newtick = current_tick + HZ;
    // Wait till we have a word to read (or 1 second has elapsed)
    while(!(CSPI_TESTREG2 & 0xF0) && (newtick > current_tick));
    if(newtick >= current_tick) {
        *buffer = CSPI_RXDATA2;
    } else {
        DEBUGF("SPI read timed out");
    }
}
