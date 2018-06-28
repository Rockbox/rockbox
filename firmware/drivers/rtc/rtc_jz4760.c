/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
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
 * Real Time Clock interface for Jz4760.
 *
 */

#include "config.h"
#include "cpu.h"
#include "rtc.h"
#include "timefuncs.h"
#include "logf.h"

#define RTC_FREQ_DIVIDER     (32768 - 1)

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
    _localtime(rtc_read_reg(RTC_RTCSR), tm);

    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    rtc_write_reg(RTC_RTCSR, mktime((struct tm*)tm));

    return 0;
}

void rtc_init(void)
{
    unsigned int cfc,hspr,rgr_1hz;

    __cpm_select_rtcclk_rtc();

    cfc = HSPR_RTCV;
    hspr = rtc_read_reg(RTC_HSPR);
    rgr_1hz = rtc_read_reg(RTC_RTCGR) & RTCGR_NC1HZ_MASK;

    if((hspr != cfc) || (rgr_1hz != RTC_FREQ_DIVIDER))
    {
        /* We are powered on for the first time !!! */

        /* Set 32768 rtc clocks per seconds */
        rtc_write_reg(RTC_RTCGR, RTC_FREQ_DIVIDER);

        /* Set minimum wakeup_n pin low-level assertion time for wakeup: 100ms */
        rtc_write_reg(RTC_HWFCR, HWFCR_WAIT_TIME(100));
        rtc_write_reg(RTC_HRCR, HRCR_WAIT_TIME(60));

        /* Reset to the default time */
        rtc_write_reg(RTC_RTCSR, 946681200); /* 01/01/2000 */

        /* start rtc */
        rtc_write_reg(RTC_RTCCR, RTCCR_RTCE);
        rtc_write_reg(RTC_HSPR, cfc);
    }

    /* clear all rtc flags */
    rtc_write_reg(RTC_HWRSR, 0);
}
