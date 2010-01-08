/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
 
/*
 * Jz OnChip Real Time Clock interface for Linux
 *
 */

#include "config.h"
#include "jz4740.h"
#include "rtc.h"
#include "timefuncs.h"
#include "logf.h"

/* Stolen from dietlibc-0.29/libugly/gmtime_r.c (GPLv2) */
#define SPD (24*60*60)
#define ISLEAP(year) (!(year%4) && ((year%100) || !(year%400)))
static void _localtime(const time_t t, struct tm *r)
{
    time_t i;
    register time_t work = t % SPD;
    static int m_to_d[12] = /* This could be shared with mktime() */
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    r->tm_sec = work % 60;
    work /= 60;
    r->tm_min = work % 60;
    r->tm_hour = work / 60;
    work = t / SPD;
    r->tm_wday = (4 + work) % 7;

    for (i=1970; ; ++i)
    {
        register time_t k = ISLEAP(i) ? 366 : 365;

        if (work >= k)
            work -= k;
        else
            break;
    }

    r->tm_year = i - 1900;
    r->tm_yday = work;

    r->tm_mday = 1;
    if (ISLEAP(i) && (work>58))
    {
        if (work==59)
            r->tm_mday=2; /* 29.2. */

        work-=1;
    }

    for (i=11; i && (m_to_d[i] > work); --i);
    r->tm_mon = i;
    r->tm_mday += work - m_to_d[i];
}

int rtc_read_datetime(struct tm *tm)
{
    _localtime(REG_RTC_RSR, tm);

    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    time_t val = mktime((struct tm*)tm);

    __cpm_start_rtc();
    udelay(100);
    REG_RTC_RSR = val;
    __cpm_stop_rtc();

    return 0;
}

void rtc_init(void)
{
    __cpm_start_rtc();
    REG_RTC_RCR = RTC_RCR_RTCE;
    udelay(70);
    while( !(REG_RTC_RCR & RTC_RCR_WRDY) );
    REG_RTC_RGR = (0x7fff | RTC_RGR_LOCK); 
    udelay(70);
    __cpm_stop_rtc();
}
