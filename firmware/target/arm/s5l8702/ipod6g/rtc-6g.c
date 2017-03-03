/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: rtc-nano2g.c 23114 2009-10-11 18:20:56Z theseven $
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
#include "rtc.h"
#include "kernel.h"
#include "system.h"
#include "pmu-target.h"
#include "timefuncs.h"

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned int i;
    unsigned char buf[7];

    pmu_read_rtc(buf);

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

    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned int i;
    unsigned char buf[7];

    buf[0] = tm->tm_sec;
    buf[1] = tm->tm_min;
    buf[2] = tm->tm_hour;
    buf[3] = tm->tm_wday;
    buf[4] = tm->tm_mday;
    buf[5] = tm->tm_mon + 1;
    buf[6] = tm->tm_year - 100;

    for (i = 0; i < sizeof(buf); i++)
         buf[i] = DEC2BCD(buf[i]);

    pmu_write_rtc(buf);

    return 0;
}

