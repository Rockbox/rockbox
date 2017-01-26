/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "timefuncs.h"

/* Choose one of: */
#define ADDR_READ       0x04
#define ADDR_WRITE      0x00
/* and one of: */
#define ADDR_ONE        0x08
#define ADDR_BURST      0x00

void rtc_init(void)
{
}
    
int rtc_read_datetime(struct tm *tm)
{
    unsigned int i;
    unsigned char buf[7];
    char command = ADDR_READ|ADDR_BURST; /* burst read from the start of the time/date reg */

    spi_block_transfer(SPI_target_RX5X348AB, &command, 1, buf, sizeof(buf));

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = BCD2DEC(buf[i]);

    tm->tm_sec = buf[0];
    tm->tm_min = buf[1];
    tm->tm_hour = buf[2];
    tm->tm_mday = buf[4];
    tm->tm_mon = buf[5] - 1;
    tm->tm_year = buf[6] + 100;
    tm->tm_yday = 0; /* Not implemented for now */

    set_day_of_week(tm);

    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned int i;
    char command = ADDR_WRITE|ADDR_BURST; /* burst read from the start of the time/date reg */
    unsigned char buf[8];

    buf[0] = command;
    buf[1] = tm->tm_sec;
    buf[2] = tm->tm_min;
    buf[3] = tm->tm_hour;
    buf[4] = tm->tm_wday;
    buf[5] = tm->tm_mday;
    buf[6] = tm->tm_mon + 1;
    buf[7] = tm->tm_year - 100;

    /* don't encode the comand byte */
    for (i = 1; i < sizeof(buf); i++)
        buf[i] = DEC2BCD(buf[i]);

    spi_block_transfer(SPI_target_RX5X348AB, buf, sizeof(buf), NULL, 0);
    return 1;
}

