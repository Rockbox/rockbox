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

static struct tm tm;

#if !CONFIG_RTC
static void fill_default_tm(struct tm *tm)
{
    tm->tm_sec = 0;
    tm->tm_min = 0;
    tm->tm_hour = 0;
    tm->tm_mday = 1;
    tm->tm_mon = 0;
    tm->tm_year = 70;
    tm->tm_wday = 1;
    tm->tm_yday = 0; /* Not implemented for now */
    tm->tm_isdst = -1; /* Not implemented for now */
}
#endif /* !CONFIG_RTC */

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
#if CONFIG_RTC
    static long timeout = 0;

    /* Don't read the RTC more than once per second */
    if (current_tick > timeout)
    {
        /* Once per second, 1/10th of a second off */
        timeout = HZ * (current_tick / HZ + 1) + HZ / 5;
        rtc_read_datetime(&tm);

        tm.tm_yday = 0; /* Not implemented for now */
        tm.tm_isdst = -1; /* Not implemented for now */
    }
#else /* No RTC */
    fill_default_tm(&tm);
#endif /* RTC */
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
    return 0;
#endif /* RTC */
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

void set_day_of_week(struct tm *tm)
{
    int y=tm->tm_year+1900;
    int d=tm->tm_mday;
    int m=tm->tm_mon;
    static const char mo[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

    if(m == 0 || m == 1) y--;
    tm->tm_wday = (d + mo[m] + y + y/4 - y/100 + y/400) % 7;
}

