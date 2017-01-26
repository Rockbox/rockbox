/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Michael Sevakis
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
#include "time.h"

/* Constants that mark Thursday, 1 January 1970 */
#define UNIX_EPOCH_DAY_NUM  134774
#define UNIX_EPOCH_YEAR     (1601 - 1900)

/* Last day number it can do */
#define MAX_DAY_NUM         551879

/* d is days since epoch start, Monday, 1 January 1601 (d = 0)
 *
 * That date is the beginning of a full 400 year cycle and so simplifies the
 * calculations a bit, not requiring offsets before divisions to shift the
 * leap year cycle.
 *
 * It can handle dates up through Sunday, 31 December 3111 (d = 551879).
 *
 * time_t can't get near the limits anyway for now but algorithm can be
 * altered slightly to increase range if even needed.
 */
static void get_tmdate(unsigned int d, struct tm *tm)
{
    static const unsigned short mon_yday[13] =
    {
        /* year day of 1st of month (non-leap)
      +31  +28  +31  +30  +31  +30  +31  +31  +30  +31  +30  +31  +31 */
        0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };

    unsigned int x0, x1, x2, x3; /* scratch variables */

    /* calculate year from day number */
    x0 = d / 1460;
    x1 = x0 / 25;
    x2 = x1 / 4;

    unsigned int y = (d - x0 + x1 - x2) / 365;

    tm->tm_year = y + UNIX_EPOCH_YEAR;          /* year - 1900 */

    /* calculate year day from year number and day number */
    x0 = y / 4;
    x1 = x0 / 25;
    x2 = x1 / 4;

    unsigned int yday = d - x0 + x1 - x2 - y * 365;

    tm->tm_yday = x3 = yday;                    /* 0..364/365 */

    /* check if leap year; adjust February->March transition if so rather
       than keeping a leap year version of mon_yday[] */
    if (y - x0 * 4 == 3 && (x0 - x1 * 25 != 24 || x1 - x2 * 4 == 3)) {
        /* retard month lookup to make year day 59 into 29 Feb, both to make
           year day 60 into 01 March, lagging one day for remainder of year */
        if (x3 >= mon_yday[2] && --x3 >= mon_yday[2]) {
            yday--;
        }
    }

    /* stab approximately at current month based on year day; advance if
       it fell short (never initially more than 1 short). */
    x0 = x3 / 32;
    if (mon_yday[x0 + 1] <= x3) {
        x0++;
    }

    tm->tm_mon  = x0;                           /* 0..11 */
    tm->tm_mday = yday - mon_yday[x0] + 1;      /* 1..31 */
    tm->tm_wday = (d + 1) % 7;                  /* 0..6 */
}

struct tm * gmtime_r(const time_t *timep, struct tm *tm)
{
    time_t t = *timep;

    int d = t / 86400;             /* day number (-24856..24855) */
    int s = t - (time_t)d * 86400; /* second # of day (0..86399) */

    if (s < 0) {
        /* round towards 0 -> floored division */
        d--;
        s += 86400;
    }

    unsigned int x;

    x = s / 3600;
    tm->tm_hour = x;    /* 0..23 */

    s -= x * 3600;
    x = s / 60;
    tm->tm_min = x;     /* 0..59 */

    s -= x * 60;
    tm->tm_sec = s;     /* 0..59 */

    /* not implemented right now */
    tm->tm_isdst = -1;

    get_tmdate(d + UNIX_EPOCH_DAY_NUM, tm);

    return tm;
}

struct tm * gmtime(const time_t *timep)
{
    static struct tm time;
    return gmtime_r(timep, &time);
}
