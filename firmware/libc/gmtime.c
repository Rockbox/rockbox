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

#define MINUTE_SECONDS      60
#define HOUR_SECONDS        3600
#define DAY_SECONDS         86400
#define WEEK_SECONDS        604800
#define YEAR_SECONDS        31536000
#define LEAP_YEAR_SECONDS   31622400

/* Days in each month */
static uint8_t days_in_month[] =
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline bool is_leapyear(int year)
{
    return (((year%4)==0) && (((year%100)!=0) || ((year%400)==0)));
}

struct tm *gmtime(const time_t *timep)
{
    static struct tm time;
    return gmtime_r(timep, &time);
}

struct tm *gmtime_r(const time_t *timep, struct tm *tm)
{
    time_t seconds = *timep;
    int year, i, mday, hour, min;

    /* weekday */
    tm->tm_wday = (seconds / DAY_SECONDS + 4) % 7;

    /* Year */
    year = 1970;
    while (seconds >= LEAP_YEAR_SECONDS)
    {
        if (is_leapyear(year)){
            seconds -= LEAP_YEAR_SECONDS;
        } else {
            seconds -= YEAR_SECONDS;
        }

        year++;
    }

    if (is_leapyear(year)) {
        days_in_month[1] = 29;
    } else {
        days_in_month[1] = 28;
        if(seconds>YEAR_SECONDS){
            year++;
            seconds -= YEAR_SECONDS;
        }
    }
    tm->tm_year = year - 1900;

    /* Month */
    for (i = 0; i < 12; i++)
    {
        if (seconds < days_in_month[i]*DAY_SECONDS){
            tm->tm_mon = i;
            break;
        }

        seconds -= days_in_month[i]*DAY_SECONDS;
    }

    /* Month Day */
    mday = seconds/DAY_SECONDS;
    seconds -= mday*DAY_SECONDS;
    tm->tm_mday = mday + 1; /* 1 ... 31 */

    /* Hour */
    hour = seconds/HOUR_SECONDS;
    seconds -= hour*HOUR_SECONDS;
    tm->tm_hour = hour;

    /* Minute */
    min = seconds/MINUTE_SECONDS;
    seconds -= min*MINUTE_SECONDS;
    tm->tm_min = min;

    /* Second */
    tm->tm_sec = seconds;

    return tm;
}

