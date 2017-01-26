/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Bertrik Sikken
 *
 * Based on code from: rtc_as3514.c
 * Copyright (C) 2007 by Barry Wardell
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
#include <stdbool.h>
#include <stdint.h>
#include "time.h"

struct tm *gmtime(const time_t *timep)
{
    static struct tm time;
    return gmtime_r(timep, &time);
}

struct tm *gmtime_r(const time_t *timep, struct tm *tm)
{
    /* Epoch start: Thursday, 1 January 1970 */
    static const unsigned short mon_yday[13] =
    {
        /* Since 1 Jan, how many days have passed this year by the 1st of
           each month or, if you like, year day of 1st of month (non-leap)
      +31  +28  +31  +30  +31  +30  +31  +31  +30  +31  +30  +31  +31 */
        0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };

    int x0, x1, x2, x3; /* scratch variables */

    time_t s = *timep;
    int d = s / 86400;

    x0 = s - (time_t)d * 86400; /* second # of day (0..86399) */
    x1 = x0 / 3600;
    tm->tm_hour = x1;

    x0 -= x1 * 3600;            /* second # of hour (0..3599) */
    x1 = x0 / 60;
    tm->tm_min = x1;

    x0 -= x1 * 60;              /* second # of minute (0..59) */
    tm->tm_sec = x0;

    /* calculate year from day number */
    x0 = (d / 365 + 1) / 4;
    x1 = (x0 + 17) / 25;
    x2 = (x1 + 3) / 4;

    int y = (d - x0 + x1 - x2) / 365;

    tm->tm_year = y + 70;                   /* 70.. */

    /* calculate year day from year number and day number */
    x0 = (y + 1) / 4;
    x1 = (x0 + 17) / 25;
    x2 = (x1 + 3) / 4;

    int yday = d - x0 + x1 - x2 - y * 365;

    tm->tm_yday = x3 = yday;                /* 0..364/365 */

    /* check if leap year; adjust February->March transition if so rather
       than keeping a leap year version of mon_yday[]; comparisons are made
       with remainders without adjusting for the offests before division made
       above */
    if (y - x0 * 4 == 2 && (x0 - x1 * 25 != 7 || x1 - x2 * 4 == 0))
    {
        /* retard month lookup to make year day 59 into 29 Feb, both to make
           year day 60 into 01 March, lagging one day for remainder of year */
        if (x3 >= mon_yday[2] && --x3 >= mon_yday[2])
            yday--;
    }

    /* stab approximately at current month based on year day; advance if
       it fell short (never initially more than 1 short). */
    x0 = x3 / 32;
    if (mon_yday[x0 + 1] <= x3)
        x0++;

    tm->tm_mon  = x0;                      /* 0..11 */
    tm->tm_mday = yday - mon_yday[x0] + 1; /* 1..31 */
    tm->tm_wday = (d + 4) % 7;             /* 0..6 */

    tm->tm_isdst = -1; /* not implemented right now */

    return tm;
}
