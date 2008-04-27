/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "spi.h"
#include "tsc2100.h"

/* Read X, Y, Z1, Z2 touchscreen coordinates. */
void tsc2100_read_values(short *x, short* y, short *z1, short *z2)
{
    int page = 0, address = 0;
    unsigned short command = 0x8000|(page << 11)|(address << 5);
    unsigned char out[] = {command >> 8, command & 0xff};
    unsigned char in[8];
    spi_block_transfer(SPI_target_TSC2100, 
                       out, sizeof(out), in, sizeof(in));

    *x = (in[0]<<8)|in[1];
    *y = (in[2]<<8)|in[3];
    *z1 = (in[4]<<8)|in[5];
    *z2 = (in[6]<<8)|in[7];
}

short tsc2100_readreg(int page, int address)
{
    unsigned short command = 0x8000|(page << 11)|(address << 5);
    unsigned char out[] = {command >> 8, command & 0xff};
    unsigned char in[2];
    spi_block_transfer(SPI_target_TSC2100, 
                       out, sizeof(out), in, sizeof(in));
    return (in[0]<<8)|in[1];
}


void tsc2100_writereg(int page, int address, short value)
{
    unsigned short command = (page << 11)|(address << 5);
    unsigned char out[4] = {command >> 8, command & 0xff,
                            value >> 8,   value & 0xff};
    spi_block_transfer(SPI_target_TSC2100, 
                       out, sizeof(out), NULL, 0);
}

void tsc2100_keyclick(void)
{
    // 1100 0100 0001 0000
    //short val = 0xC410;
    tsc2100_writereg(TSAC2_PAGE, TSAC2_ADDRESS, tsc2100_readreg(TSAC2_PAGE, TSAC2_ADDRESS)&0x8000);
}
