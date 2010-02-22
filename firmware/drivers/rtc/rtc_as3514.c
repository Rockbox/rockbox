/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#include "config.h"
#include "rtc.h"
#include "as3514.h"
#include "ascodec.h"

/* AMS Sansas start counting from Jan 1st 1970 instead of 1980 (not as3525v2) */
#if (CONFIG_CPU==AS3525)
#define SECS_ADJUST 315532800   /* seconds between 1970-1-1 and 1980-1-1 */
#elif (CONFIG_CPU==AS3525v2)
#define SECS_ADJUST 315532800 - (2*365*24*3600) - 26*(24*3600) + 7*3600 + 25*60
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
}

int rtc_read_datetime(struct tm *tm)
{
    char tmp[4];
    int i, year, mday, hour, min;
    unsigned int seconds;

    /* RTC_AS3514's slave address is 0x46*/
    for (i = 0; i < 4; i++){
        tmp[i] = ascodec_read(AS3514_RTC_0 + i);
    }
    seconds = tmp[0] + (tmp[1]<<8) + (tmp[2]<<16) + (tmp[3]<<24);
    seconds -= SECS_ADJUST;

    /* Convert seconds since Jan-1-1980 to format compatible with
     * get_time() from firmware/common/timefuncs.c */

    /* weekday */
    tm->tm_wday = ((seconds % WEEK_SECONDS) / DAY_SECONDS + 2) % 7;

    /* Year */
    year = 1980;
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
    tm->tm_year = year%100 + 100;

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

    return 7;
}

int rtc_write_datetime(const struct tm *tm)
{
    int i, year;
    unsigned int year_days = 0;
    unsigned int month_days = 0;
    unsigned int seconds = 0;

    year = 2000 + tm->tm_year - 100;

    if(is_leapyear(year)) {
        days_in_month[1] = 29;
    } else {
        days_in_month[1] = 28;
    }
    
    /* Number of days in months gone by this year*/
    for(i = 0; i < tm->tm_mon; i++){
        month_days += days_in_month[i];
    }
    
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

    /* Send data to RTC */
    for (i=0; i<4; i++){
        ascodec_write(AS3514_RTC_0 + i, ((seconds >> (8 * i)) & 0xff));
    }
    return 1;
}

