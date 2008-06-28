/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: rtc_as3514.c 12131 2007-01-27 20:48:48Z dan_a $
 *
 * Copyright (C) 2007 by Jonathan Gordon
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

#include "config.h"
#include "spi.h"
#include "rtc.h"
#include <stdbool.h>
/* Choose one of: */
#define ADDR_READ       0x04
#define ADDR_WRITE      0x00
/* and one of: */
#define ADDR_ONE        0x08
#define ADDR_BURST      0x00
void rtc_init(void)
{
}
    
int rtc_read_datetime(unsigned char* buf)
{
    char command = ADDR_READ|ADDR_BURST; /* burst read from the start of the time/date reg */
    spi_block_transfer(SPI_target_RX5X348AB,
                       &command, 1, buf, 7);
    return 1;
}
int rtc_write_datetime(unsigned char* buf)
{
    char command = ADDR_WRITE|ADDR_BURST; /* burst read from the start of the time/date reg */
    char data[8];
    int i;
    data[0] = command;
    for (i=1;i<8;i++)
        data[i] = buf[i-1];
    spi_block_transfer(SPI_target_RX5X348AB,
                       data, 8, NULL, 0);
    return 1;
}
