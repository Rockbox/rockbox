/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

/* The PMU keeps seconds, minutes, hours, day, month and year (since 2000)
 * in binary, and no weekday; see pmu_read_rtc() */

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned char buf[6];

    pmu_read_rtc(buf);

    tm->tm_sec = buf[0];
    tm->tm_min = buf[1];
    tm->tm_hour = buf[2];
    tm->tm_mday = buf[3];
    tm->tm_mon = buf[4] - 1;
    tm->tm_year = buf[5] + 100;
    tm->tm_yday = 0; /* Not implemented for now */

    set_day_of_week(tm);
    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned char buf[6];

    buf[0] = tm->tm_sec;
    buf[1] = tm->tm_min;
    buf[2] = tm->tm_hour;
    buf[3] = tm->tm_mday;
    buf[4] = tm->tm_mon + 1;
    buf[5] = tm->tm_year - 100;

    pmu_write_rtc(buf);
    return 0;
}
