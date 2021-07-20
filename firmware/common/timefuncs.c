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
#ifdef HAVE_RTC_IRQ
#include "rtc-target.h"
#endif
#include "timefuncs.h"
#include "debug.h"

static struct tm tm;

time_t dostime_mktime(uint16_t dosdate, uint16_t dostime)
{
    /* this knows our mktime() only uses these struct tm fields */
    struct tm tm;
    tm.tm_sec  = ((dostime      ) & 0x1f) * 2;
    tm.tm_min  = ((dostime >>  5) & 0x3f);
    tm.tm_hour = ((dostime >> 11)       );
    tm.tm_mday = ((dosdate      ) & 0x1f);
    tm.tm_mon  = ((dosdate >>  5) & 0x0f) - 1;
    tm.tm_year = ((dosdate >>  9)       ) + 80;

    return mktime(&tm);
}

void dostime_localtime(time_t time, uint16_t* dosdate, uint16_t* dostime)
{
    struct tm tm;
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    gmtime_r(&time, &tm);
#else
    localtime_r(&time, &tm);
#endif

    *dostime = ((tm.tm_sec  /  2) <<  0)|
               ((tm.tm_min      ) <<  5)|
               ((tm.tm_hour     ) << 11);
    *dosdate = ((tm.tm_mday     ) <<  0)|
               ((tm.tm_mon  +  1) <<  5)|
               ((tm.tm_year - 80) <<  9);
}

#if !CONFIG_RTC
static inline bool rtc_dirty(void)
{
    return true;
}

static inline int rtc_read_datetime(struct tm *tm)
{
    tm->tm_sec = 0;
    tm->tm_min = 0;
    tm->tm_hour = 0;
    tm->tm_mday = 1;
    tm->tm_mon = 0;
    tm->tm_year = 70;
    tm->tm_wday = 4;
    tm->tm_yday = 0;
    return 1;
}
#endif /* !CONFIG_RTC */

#if CONFIG_RTC
bool valid_time(const struct tm *tm)
{
    if (!tm || (tm->tm_hour < 0 || tm->tm_hour > 23 ||
        tm->tm_sec < 0 || tm->tm_sec > 59 || 
        tm->tm_min < 0 || tm->tm_min > 59 ||
        tm->tm_year < 100 || tm->tm_year > 199 ||
        tm->tm_mon < 0 || tm->tm_mon > 11 ||
        tm->tm_wday < 0 || tm->tm_wday > 6 ||
        tm->tm_mday < 1 || tm->tm_mday > 31))
        return false;
    else
        return true;
}

/* Don't read the RTC more than once per second
 * returns true if the rtc needs to be read
 * targets may override with their own implementation
 */
#ifndef HAVE_RTC_IRQ
static inline bool rtc_dirty(void)
{
    static long timeout = 0;

    /* Don't read the RTC more than once per second */
    if (TIME_AFTER(current_tick, timeout))
    {
        /* Once per second, 1/5th of a second off */
        timeout = current_tick / HZ * HZ + 6*HZ / 5;
        return true;
    }

    return false;
}
#endif /* HAVE_RTC_IRQ */
#endif /* CONFIG_RTC */

struct tm *get_time(void)
{
    if (rtc_dirty())
    {
        rtc_read_datetime(&tm);
        tm.tm_isdst = -1; /* Not implemented for now */
    }

    return &tm;
}

int set_time(const struct tm *tm)
{
#if CONFIG_RTC
    int rc;

    if (valid_time(tm))
    {
        rc = rtc_write_datetime(tm);

        if (rc < 0)
            return -1;
        else
            return 0;
    }
    else
    {
        return -2;
    }
#else /* No RTC */
    (void)tm;
    return -1;
#endif /* RTC */
}

#if CONFIG_RTC
void set_day_of_week(struct tm *tm)
{
    int y=tm->tm_year+1900;
    int d=tm->tm_mday;
    int m=tm->tm_mon;
    static const char mo[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    if(m == 0 || m == 1) y--;
    tm->tm_wday = (d + mo[m] + y + y/4 - y/100 + y/400) % 7;
}

void set_day_of_year(struct tm *tm)
{
    int y=tm->tm_year+1900;
    int d=tm->tm_mday;
    int m=tm->tm_mon;
    d+=m*30;
    if( ( (m>1) && !(y%4) ) &&  (  (y%100) ||  !(y%400)  )  )
        d++;
    if(m>6)
    {
        d+=4;
        m-=7;
    }
    tm->tm_yday = d + ((m+1) /2);
}
#endif /* CONFIG_RTC */

