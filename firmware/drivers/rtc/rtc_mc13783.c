/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
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
#include "config.h"
#include "system.h"
#include "rtc.h"
#include "mc13783.h"

/* NOTE: Defined the base to be original firmware compatible if needed -
 * ie. the day and year as it would interpret a DAY register value of zero. */

/* None of this code concerns itself with (year mod 100) = 0 leap year
 * exceptions because all (year mod 4) = 0 years in the relevant range are
 * leap years. A base year of 1901 to an end date of 28 Feb 2100 are ok. */

#ifdef TOSHIBA_GIGABEAT_S
    /* Gigabeat S seems to be 1 day behind the ususual - this will
     * make the RTC match file dates created by retailos. */
    #define RTC_BASE_WDAY       1      /* Monday */
    #define RTC_BASE_YDAY       364    /* 31 Dec */
    #define RTC_BASE_YEAR       1979
#elif 1
    #define RTC_BASE_WDAY       2      /* Tuesday */
    #define RTC_BASE_YDAY       0      /* 01 Jan */
    #define RTC_BASE_YEAR       1980
#else
    #define RTC_BASE_WDAY       4      /* Thursday */
    #define RTC_BASE_YDAY       0      /* 01 Jan */
    #define RTC_BASE_YEAR       1970
#endif

/* Reference year for leap calculation - year that is on or before the base
 * year and immediately follows a leap year */
#define RTC_REF_YEAR_OFFS    ((RTC_BASE_YEAR - 1) & 3)
#define RTC_REF_YEAR         (RTC_BASE_YEAR - RTC_REF_YEAR_OFFS)

enum rtc_registers_indexes
{
    RTC_REG_TIME = 0,
    RTC_REG_DAY,
    RTC_REG_TIME2,
    RTC_NUM_REGS_RD = 3,
    RTC_NUM_REGS_WR = 2,
};

static const unsigned char rtc_registers[RTC_NUM_REGS_RD] =
{
    [RTC_REG_TIME]  = MC13783_RTC_TIME,
    [RTC_REG_DAY]   = MC13783_RTC_DAY,
    [RTC_REG_TIME2] = MC13783_RTC_TIME,
};

/* was it an alarm that triggered power on ? */
static bool alarm_start = false;

static const unsigned short month_table[13] =
{
    /* Since 1 Jan, how many days have passed this year? (non-leap)
    +31   28   31   30   31   30   31   31   30   31   30   31 */
    0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

static bool read_time_and_day(uint32_t regs[RTC_NUM_REGS_RD])
{
    /* Read time, day, time - 2nd read of time should be the same or
     * greater */
    do
    {
        if (mc13783_read_regs(rtc_registers, regs,
                              RTC_NUM_REGS_RD) < RTC_NUM_REGS_RD)
        {
            /* Couldn't read registers */
            return false;
        }
    }
    /* If TOD counter turned over - reread */
    while (regs[RTC_REG_TIME2] < regs[RTC_REG_TIME]);

    return true;
}

/** Public APIs **/
void rtc_init(void)
{
    /* only needs to be polled on startup */
    if (mc13783_read(MC13783_INTERRUPT_STATUS1) & MC13783_TODAI)
    {
        alarm_start = true;
        mc13783_write(MC13783_INTERRUPT_STATUS1, MC13783_TODAI);
    }
}

int rtc_read_datetime(struct tm *tm)
{
    uint32_t regs[RTC_NUM_REGS_RD];
    int year, month, yday, x, n;

    if (!read_time_and_day(regs))
        return 0;

    /* TOD: 0 to 86399 */
    x = regs[RTC_REG_TIME];

    /* Hour */
    n = x / 3600;
    tm->tm_hour = n;

    /* Minute */
    x -= n*3600;
    n = x / 60;
    tm->tm_min = n;

    /* Second */
    x -= n*60;
    tm->tm_sec = x;

    /* DAY: 0 to 32767 */
    x = regs[RTC_REG_DAY];

    /* Weekday */
    tm->tm_wday = (x + RTC_BASE_WDAY) % 7;

    /* Year */
    x += RTC_REF_YEAR_OFFS*365 + RTC_BASE_YDAY;

    /* Lag year increment by subtracting leaps since the reference year
     * on 31 Dec of each leap year, essentially removing them from the
     * calculation */
    n = (x + 1) / 1461;
    year = (x - n) / 365;

    /* Year day */
    yday = x - n - year*365;

    /* If (x + 1) mod 1461 == 0, then it is yday 365 of a leap year */
    if (n * 1461 - 1 == x)
        yday++;

    tm->tm_yday = x = yday;

    if (((year + RTC_REF_YEAR) & 3) == 0 && x >= month_table[2])
    {
        if (x > month_table[2])
            yday--; /* 1 Mar or after; lag date by one day */

        x--; /* 29 Feb or after, lag month by one day */
    }

    /* Get the current month */
    month = x >> 5; /* yday / 32, close enough */

    if (month_table[month + 1] <= x)
        month++; /* Round to next */

    tm->tm_mday = yday - month_table[month] + 1; /* 1 to 31 */
    tm->tm_mon = month; /* 0 to 11 */

    /* {BY to (BY+89 or 90)} - 1900 */
    tm->tm_year = year + RTC_REF_YEAR - 1900;

    return 7;
}

int rtc_write_datetime(const struct tm *tm)
{
    uint32_t regs[RTC_NUM_REGS_WR];
    int year, month, x;

    /* Convert time of day into seconds since midnight */
    x = tm->tm_sec + tm->tm_min*60 + tm->tm_hour*3600;

    /* Keep in range (it throws off the PMIC counters otherwise) */
    regs[RTC_REG_TIME] = MAX(0, MIN(86399, x));

    year = tm->tm_year + 1900;

    /* Get the number of days elapsed from 1 Jan of reference year to 1 Jan of
     * this year */
    x = year - RTC_REF_YEAR;
    x = x*365 + (x >> 2);

    /* Add the number of days passed this year since 1 Jan and offset by the
     * base yearday and the reference offset in days from the base */
    month = tm->tm_mon;
    x += month_table[month] + tm->tm_mday - RTC_REF_YEAR_OFFS*365
            - RTC_BASE_YDAY;

    if ((year & 3) != 0 || month < 2) {
        /* Sub one day because tm_mday starts at 1, otherwise the offset is
         * required because of 29 Feb */
        x--;
    }

    /* Keep in range */
    regs[RTC_REG_DAY] = MAX(0, MIN(32767, x));

    if (mc13783_write_regs(rtc_registers, regs, RTC_NUM_REGS_WR)
        == RTC_NUM_REGS_WR)
    {
        return 7;
    }

    return 0;
}

bool rtc_check_alarm_flag(void)
{
    /* We don't need to do anything special if it has already fired */
    return false;
}

void rtc_enable_alarm(bool enable)
{
    if (enable)
        mc13783_clear(MC13783_INTERRUPT_MASK1, MC13783_TODAM);
    else
        mc13783_set(MC13783_INTERRUPT_MASK1, MC13783_TODAM);
}

bool rtc_check_alarm_started(bool release_alarm)
{
    bool rc = alarm_start;

    if (release_alarm)
        alarm_start = false;

    return rc;
}

void rtc_set_alarm(int h, int m)
{
    uint32_t regs[RTC_NUM_REGS_RD];
    uint32_t tod;

    if (!read_time_and_day(regs))
        return;

    tod = h*3600 + m*60;

    if (tod < regs[RTC_REG_TIME])
        regs[RTC_REG_DAY]++;

    mc13783_write(MC13783_RTC_DAY_ALARM, regs[RTC_REG_DAY]);
    mc13783_write(MC13783_RTC_ALARM, tod);
}

void rtc_get_alarm(int *h, int *m)
{
    uint32_t tod = mc13783_read(MC13783_RTC_ALARM);
    *h = tod / 3600;
    *m = tod % 3600 / 60;
}
