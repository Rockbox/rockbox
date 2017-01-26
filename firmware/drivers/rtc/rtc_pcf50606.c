/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing, Uwe Freese, Laurent Baum
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
#include "i2c.h"
#include "rtc.h"
#include "kernel.h"
#include "system.h"
#include "pcf50606.h"
#include "timefuncs.h"

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned int i;
    int rc, oldlevel;
    unsigned char buf[7];

    oldlevel = disable_irq_save();

    rc = pcf50606_read_multiple(0x0a, buf, sizeof(buf));

    restore_irq(oldlevel);

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = BCD2DEC(buf[i]);

    tm->tm_sec = buf[0];
    tm->tm_min = buf[1];
    tm->tm_hour = buf[2];
    tm->tm_mday = buf[4];
    tm->tm_mon = buf[5] - 1;
    tm->tm_yday = 0; /* Not implemented for now */
#ifdef IRIVER_H300_SERIES 
    /* Special kludge to coexist with the iriver firmware. The iriver firmware
       stores the date as 1965+nn, and allows a range of 1980..2064. We use
       1964+nn here to make leap years work correctly, so the date will be one
       year off in the iriver firmware but at least won't be reset anymore. */
    tm->tm_year = buf[6] + 64;
#else /* Not IRIVER_H300_SERIES */
    tm->tm_year = buf[6] + 100;
#endif /* IRIVER_H300_SERIES */

    set_day_of_week(tm);

    return rc;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned int i;
    int rc, oldlevel;
    unsigned char buf[7];

    buf[0] = tm->tm_sec;
    buf[1] = tm->tm_min;
    buf[2] = tm->tm_hour;
    buf[3] = tm->tm_wday;
    buf[4] = tm->tm_mday;
    buf[5] = tm->tm_mon + 1;
#ifdef IRIVER_H300_SERIES
    /* Iriver firmware compatibility kludge, see rtc_read_datetime(). */
    buf[6] = tm->tm_year - 64;
#else /* Not IRIVER_H300_SERIES */
    buf[6] = tm->tm_year - 100;
#endif /* IRIVER_H300_SERIES */

    for (i = 0; i < sizeof(buf); i++)
         buf[i] = DEC2BCD(buf[i]);

    oldlevel = disable_irq_save();

    rc = pcf50606_write_multiple(0x0a, buf, sizeof(buf));

    restore_irq(oldlevel);

    return rc;
}

