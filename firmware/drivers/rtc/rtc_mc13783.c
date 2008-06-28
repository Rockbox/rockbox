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

/* Days passed since midnight 01 Jan, 1601 to midnight on the base date. */
#ifdef TOSHIBA_GIGABEAT_S
    /* Gigabeat S seems to be 1 day behind the ususual - this will
     * make the RTC match file dates created by retailos. */
    #define RTC_BASE_DAY_COUNT  138425
    #define RTC_BASE_MONTH      12
    #define RTC_BASE_DAY        31
    #define RTC_BASE_YEAR       1979
#elif 1
    #define RTC_BASE_DAY_COUNT  138426
    #define RTC_BASE_MONTH      1
    #define RTC_BASE_DAY        1
    #define RTC_BASE_YEAR       1980
#else
    #define RTC_BASE_DAY_COUNT  134774
    #define RTC_BASE_MONTH      1
    #define RTC_BASE_DAY        1
    #define RTC_BASE_YEAR       1970
#endif

enum rtc_buffer_field_indexes
{
    RTC_I_SECONDS = 0,
    RTC_I_MINUTES,
    RTC_I_HOURS,
    RTC_I_WEEKDAY,
    RTC_I_DAY,
    RTC_I_MONTH,
    RTC_I_YEAR,
    RTC_NUM_FIELDS,
};

enum rtc_registers_indexes
{
    RTC_REG_TIME = 0,
    RTC_REG_DAY,
    RTC_REG_TIME2,
    RTC_NUM_REGS,
};

/* was it an alarm that triggered power on ? */
static bool alarm_start = false;

static const unsigned char rtc_registers[RTC_NUM_REGS] =
{
    [RTC_REG_TIME]  = MC13783_RTC_TIME,
    [RTC_REG_DAY]   = MC13783_RTC_DAY,
    [RTC_REG_TIME2] = MC13783_RTC_TIME,
};

static const unsigned char month_table[2][12] =
{
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

static inline void to_bcd(unsigned char *bcd, const unsigned char *buf,
                          int len)
{
    while (len-- > 0)
    {
        unsigned char d = *buf++;
        *bcd++ = ((d / 10) << 4) | (d % 10);
    }
}

static inline void from_bcd(unsigned char *buf, const unsigned char *bcd,
                            int len)
{
    while (len-- > 0)
    {
        unsigned char d = *bcd++;
        *buf++ = ((d >> 4) & 0x0f) * 10 + (d & 0xf);
    }
}

/* Get number of leaps since the reference date of 1601/01/01 */
static int get_leap_count(int d)
{
    int lm = (d + 1) / 146097;
    int lc = (d + 1 - lm) / 36524;
    int ly = (d + 1 - lm + lc) / 1461;
    return ly - lc + lm;
}

static int is_leap_year(int y)
{
    return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0) ? 1 : 0;
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

int rtc_read_datetime(unsigned char* buf)
{
    uint32_t regs[RTC_NUM_REGS];
    int year, leap, month, day;

    /* Read time, day, time - 2nd read of time should be the same or
     * greater */
    do
    {
        if (mc13783_read_regset(rtc_registers, regs,
                                RTC_NUM_REGS) < RTC_NUM_REGS)
        {
            /* Couldn't read registers */
            return 0;
        }
    }
    /* If TOD counter turned over - reread */
    while (regs[RTC_REG_TIME2] < regs[RTC_REG_TIME]);

    /* TOD: = 0 to 86399 */
    buf[RTC_I_HOURS] = regs[RTC_REG_TIME] / 3600;
    regs[RTC_REG_TIME] -= buf[RTC_I_HOURS]*3600;

    buf[RTC_I_MINUTES] = regs[RTC_REG_TIME] / 60;
    regs[RTC_REG_TIME] -= buf[RTC_I_MINUTES]*60;

    buf[RTC_I_SECONDS] = regs[RTC_REG_TIME];

    /* DAY: 0 to 32767 */
    day = regs[RTC_REG_DAY] + RTC_BASE_DAY_COUNT;

    /* Weekday */
    buf[RTC_I_WEEKDAY] = (day + 1) % 7; /* 1601/01/01 = Monday */

    /* Get number of leaps for today */
    leap = get_leap_count(day);
    year = (day - leap) / 365;

    /* Get number of leaps for yesterday */
    leap = get_leap_count(day - 1);

    /* Get day number for year 0-364|365 */
    day = day - leap - year * 365;

    year += 1601;

    /* Get the current month */
    leap = is_leap_year(year);

    for (month = 0; month < 12; month++)
    {
        int days = month_table[leap][month];

        if (day < days)
            break;

        day -= days;
    }

    buf[RTC_I_DAY] = day + 1; /* 1 to 31 */
    buf[RTC_I_MONTH] = month + 1; /* 1 to 12 */
    buf[RTC_I_YEAR] = year % 100;

    to_bcd(buf, buf, RTC_NUM_FIELDS);

    return 7;
}

int rtc_write_datetime(unsigned char* buf)
{
    uint32_t regs[2];
    unsigned char fld[RTC_NUM_FIELDS];
    int year, leap, month, day, i, base_yearday;

    from_bcd(fld, buf, RTC_NUM_FIELDS);

    regs[RTC_REG_TIME] = fld[RTC_I_SECONDS] +
                         fld[RTC_I_MINUTES]*60 +
                         fld[RTC_I_HOURS]*3600;

    year = fld[RTC_I_YEAR];

    if (year < RTC_BASE_YEAR - 1900)
        year += 2000;
    else
        year += 1900;

    /* Get number of leaps for day before base */
    leap = get_leap_count(RTC_BASE_DAY_COUNT - 1);

    /* Get day number for base year 0-364|365 */
    base_yearday = RTC_BASE_DAY_COUNT - leap -
                    (RTC_BASE_YEAR - 1601) * 365;

    /* Get the number of days elapsed from reference */
    for (i = RTC_BASE_YEAR, day = 0; i < year; i++)
    {
        day += is_leap_year(i) ? 366 : 365;
    }

    /* Find the number of days passed this year up to the 1st of the
     * month. */
    leap = is_leap_year(year);
    month = fld[RTC_I_MONTH] - 1;

    for (i = 0; i < month; i++)
    {
        day += month_table[leap][i];
    }

    regs[RTC_REG_DAY] = day + fld[RTC_I_DAY] - 1 - base_yearday;

    if (mc13783_write_regset(rtc_registers, regs, 2) == 2)
    {
        return RTC_NUM_FIELDS;
    }

    return 0;
}

bool rtc_check_alarm_flag(void)
{
    /* We don't need to do anything special if it has already fired */
    return false;
}

bool rtc_enable_alarm(bool enable)
{
    if (enable)
        mc13783_clear(MC13783_INTERRUPT_MASK1, MC13783_TODAM);
    else
        mc13783_set(MC13783_INTERRUPT_MASK1, MC13783_TODAM);

    return false;
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
    int day = mc13783_read(MC13783_RTC_DAY);
    int tod = mc13783_read(MC13783_RTC_TIME);

    if (h*3600 + m*60 < tod)
        day++;

    mc13783_write(MC13783_RTC_DAY_ALARM, day);
    mc13783_write(MC13783_RTC_ALARM, h*3600 + m*60);
}

void rtc_get_alarm(int *h, int *m)
{
    int tod = mc13783_read(MC13783_RTC_ALARM);
    *h = tod / 3600;
    *m = tod % 3600 / 60;
}

