/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

#include "system.h"
#include "rtc.h"
#include "timefuncs.h"
#include "rtc-imx233.h"

#if defined(SANSA_FUZEPLUS)
#define SECS_ADJUST 315532800   /* seconds between 1970-1-1 and 1980-1-1 */
#else
#define SECS_ADJUST 0
#endif

#define MINUTE_SECONDS      60
#define HOUR_SECONDS        3600
#define DAY_SECONDS         86400
#define WEEK_SECONDS        604800
#define YEAR_SECONDS        31536000
#define LEAP_YEAR_SECONDS   31622400

/* Days in each month */
static unsigned int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline bool is_leapyear(int year)
{
    if( ((year%4)==0) && (((year%100)!=0) || ((year%400)==0)) )
        return true;
    else
        return false;
}

void rtc_init(void)
{
    imx233_rtc_init();
}

int rtc_read_datetime(struct tm *tm)
{
    uint32_t seconds = imx233_rtc_read_seconds() - SECS_ADJUST;
    #ifdef SANSA_FUZEPLUS
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary. */
    seconds += imx233_rtc_read_persistent(2);
    #else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    #endif

    /* Convert seconds since 00:00 1/1/xxxx (xxxx=year) */

    /* weekday */
    tm->tm_wday = ((seconds % WEEK_SECONDS) / DAY_SECONDS + 2) % 7;

    /* Year */
    int year = 1980;
    while(seconds >= LEAP_YEAR_SECONDS)
    {
        if(is_leapyear(year))
            seconds -= LEAP_YEAR_SECONDS;
        else
            seconds -= YEAR_SECONDS;

        year++;
    }

    if(is_leapyear(year))
        days_in_month[1] = 29;
    else
    {
        days_in_month[1] = 28;
        if(seconds>YEAR_SECONDS)
        {
            year++;
            seconds -= YEAR_SECONDS;
        }
    }
    tm->tm_year = year % 100 + 100;

    /* Month */
    for(int i = 0; i < 12; i++)
    {
        if(seconds < days_in_month[i] * DAY_SECONDS)
        {
            tm->tm_mon = i;
            break;
        }

        seconds -= days_in_month[i] * DAY_SECONDS;
    }

    /* Month Day */
    int mday = seconds / DAY_SECONDS;
    seconds -= mday * DAY_SECONDS;
    tm->tm_mday = mday + 1; /* 1 ... 31 */

    /* Hour */
    int hour = seconds / HOUR_SECONDS;
    seconds -= hour*HOUR_SECONDS;
    tm->tm_hour = hour;

    /* Minute */
    int min = seconds / MINUTE_SECONDS;
    seconds -= min*MINUTE_SECONDS;
    tm->tm_min = min;

    /* Second */
    tm->tm_sec = seconds;

    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    int i, year;
    unsigned int year_days = 0;
    unsigned int month_days = 0;
    unsigned int seconds = 0;

    year = 2000 + tm->tm_year - 100;

    if(is_leapyear(year))
        days_in_month[1] = 29;
    else
        days_in_month[1] = 28;

    /* Number of days in months gone by this year*/
    for(i = 0; i < tm->tm_mon; i++)
        month_days += days_in_month[i];

    /* Number of days in years gone by since 1-Jan-1980 */
    year_days = 365*(tm->tm_year-100+20) + (tm->tm_year-100-1)/4 + 6;

    /* Convert to seconds since 1-Jan-1980 */
    seconds = tm->tm_sec
            + tm->tm_min*MINUTE_SECONDS
            + tm->tm_hour*HOUR_SECONDS
            + (tm->tm_mday-1)*DAY_SECONDS
            + month_days*DAY_SECONDS
            + year_days*DAY_SECONDS;
    seconds += SECS_ADJUST;

    #ifdef SANSA_FUZEPLUS
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary.
     * NOTE: the OF uses this mechanism to prevent roll back in time. Although
     *       Rockbox will handle a negative PERSISTENT2 value, the OF will detect
     *       it and won't return in time before SECONDS */
    imx233_rtc_write_persistent(2, seconds - imx233_rtc_read_seconds());
    #else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    imx233_rtc_write_seconds(seconds);
    #endif
    return 0;
}

void rtc_set_alarm(int h, int m)
{
    (void) h;
    (void) m;
}

void rtc_get_alarm(int *h, int *m)
{
    (void) h;
    (void) m;
}

void rtc_enable_alarm(bool enable)
{
    (void) enable;
}

bool rtc_check_alarm_started(bool release_alarm)
{
    (void) release_alarm;
    return false;
}

bool rtc_check_alarm_flag(void)
{
    return false;
}
