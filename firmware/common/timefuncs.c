/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

#include <stdio.h> /* get NULL */
#include "config.h"

#include "kernel.h"
#include "rtc.h"
#include "timefuncs.h"
#include "debug.h"

#ifndef SIMULATOR
static struct tm tm;
#endif

bool valid_time(const struct tm *tm)
{
    if (tm->tm_hour < 0 || tm->tm_hour > 23 ||
        tm->tm_sec < 0 || tm->tm_sec > 59 || 
        tm->tm_min < 0 || tm->tm_min > 59 ||
        tm->tm_year < 100 || tm->tm_year > 199 ||
        tm->tm_mon < 0 || tm->tm_mon > 11 ||
        tm->tm_wday < 0 || tm->tm_wday > 6 ||
        tm->tm_mday < 1 || tm->tm_mday > 31)
        return false;
    else
        return true;
}

struct tm *get_time(void)
{
#ifndef SIMULATOR
#if CONFIG_RTC
    static long timeout = 0;

    /* Don't read the RTC more than once per second */
    if (current_tick > timeout)
    {
        /* Once per second, 1/10th of a second off */
        timeout = HZ * (current_tick / HZ + 1) + HZ / 5;
#if CONFIG_RTC != RTC_JZ47XX
        char rtcbuf[7];
        rtc_read_datetime(rtcbuf);

        tm.tm_sec = ((rtcbuf[0] & 0x70) >> 4) * 10 + (rtcbuf[0] & 0x0f);
        tm.tm_min = ((rtcbuf[1] & 0x70) >> 4) * 10 + (rtcbuf[1] & 0x0f);
        tm.tm_hour = ((rtcbuf[2] & 0x30) >> 4) * 10 + (rtcbuf[2] & 0x0f);
        tm.tm_wday = rtcbuf[3] & 0x07;
        tm.tm_mday = ((rtcbuf[4] & 0x30) >> 4) * 10 + (rtcbuf[4] & 0x0f);
        tm.tm_mon = ((rtcbuf[5] & 0x10) >> 4) * 10 + (rtcbuf[5] & 0x0f) - 1;
#ifdef IRIVER_H300_SERIES 
     /* Special kludge to coexist with the iriver firmware. The iriver firmware
        stores the date as 1965+nn, and allows a range of 1980..2064. We use
        1964+nn here to make leap years work correctly, so the date will be one
        year off in the iriver firmware but at least won't be reset anymore. */
        tm.tm_year = ((rtcbuf[6] & 0xf0) >> 4) * 10 + (rtcbuf[6] & 0x0f) + 64;
#else
        tm.tm_year = ((rtcbuf[6] & 0xf0) >> 4) * 10 + (rtcbuf[6] & 0x0f) + 100;
#endif

        tm.tm_yday = 0; /* Not implemented for now */
        tm.tm_isdst = -1; /* Not implemented for now */
#else
        rtc_read_datetime((unsigned char*)&tm);
#endif
    }
#else
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    tm.tm_mday = 1;
    tm.tm_mon = 0;
    tm.tm_year = 70;
    tm.tm_wday = 1;
    tm.tm_yday = 0; /* Not implemented for now */
    tm.tm_isdst = -1; /* Not implemented for now */
#endif
    return &tm;
#else
    time_t now = time(NULL);
    return localtime(&now);
#endif
}

int set_time(const struct tm *tm)
{
#if CONFIG_RTC
    int rc;
    char rtcbuf[7];
    
    if (valid_time(tm))
    {
        rtcbuf[0]=((tm->tm_sec/10) << 4) | (tm->tm_sec%10);
        rtcbuf[1]=((tm->tm_min/10) << 4) | (tm->tm_min%10);
        rtcbuf[2]=((tm->tm_hour/10) << 4) | (tm->tm_hour%10);
        rtcbuf[3]=tm->tm_wday;
        rtcbuf[4]=((tm->tm_mday/10) << 4) | (tm->tm_mday%10);
        rtcbuf[5]=(((tm->tm_mon+1)/10) << 4) | ((tm->tm_mon+1)%10);
#ifdef IRIVER_H300_SERIES
        /* Iriver firmware compatibility kludge, see get_time(). */
        rtcbuf[6]=(((tm->tm_year-64)/10) << 4) | ((tm->tm_year-64)%10);
#else
        rtcbuf[6]=(((tm->tm_year-100)/10) << 4) | ((tm->tm_year-100)%10);
#endif

        rc = rtc_write_datetime(rtcbuf);

        if (rc < 0)
            return -1;
        else
            return 0;
    }
    else
    {
        return -2;
    }
#else
    (void)tm;
    return 0;
#endif
}

#if CONFIG_RTC
/* mktime() code taken from lynx-2.8.5 source, written
 by Philippe De Muyter <phdm@macqel.be> */
time_t mktime(struct tm *t)
{
    short month, year;
    time_t result;
    static int m_to_d[12] =
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    month = t->tm_mon;
    year = t->tm_year + month / 12 + 1900;
    month %= 12;
    if (month < 0)
    {
        year -= 1;
        month += 12;
    }
    result = (year - 1970) * 365 + (year - 1969) / 4 + m_to_d[month];
    result = (year - 1970) * 365 + m_to_d[month];
    if (month <= 1)
        year -= 1;
    result += (year - 1968) / 4;
    result -= (year - 1900) / 100;
    result += (year - 1600) / 400;
    result += t->tm_mday;
    result -= 1;
    result *= 24;
    result += t->tm_hour;
    result *= 60;
    result += t->tm_min;
    result *= 60;
    result += t->tm_sec;
    return(result);
}
#endif
