/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 Aidan MacDonald
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
#include "rtc.h"
#include "timefuncs.h"
#include "regs/stm32h743/rtc.h"
#include "regs/stm32h743/rcc.h"
#include "regs/stm32h743/pwr.h"

void rtc_init(void)
{
    reg_writef(RCC_APB4ENR, RTCAPBEN(1));

    /* Initialize RTC if needed */
    if (!reg_readf(RTC_ISR, INITS))
    {
        struct tm tm = {
            .tm_hour = 0,
            .tm_min = 0,
            .tm_sec = 0,
            .tm_year = 101,
            .tm_mon = 0,
            .tm_mday = 1,
            .tm_wday = 1,
            .tm_isdst = 0,
        };

        rtc_write_datetime(&tm);
    }
}

int rtc_read_datetime(struct tm *tm)
{
    /*
     * Wait for RSF bit to indicate time/date registers are valid.
     * But if the calendar is not initialized, then don't wait as
     * they will never be valid.
     */
    bool valid = false;
    while (!valid)
    {
        uint32_t isr = reg_read(RTC_ISR);

        if (!reg_vreadf(isr, RTC_ISR, INITS))
            break;

        if (reg_vreadf(isr, RTC_ISR, RSF))
            valid = true;
    }

    if (valid)
    {
        uint32_t tr = reg_read(RTC_TR);
        uint32_t dr = reg_read(RTC_DR);

        tm->tm_sec = reg_vreadf(tr, RTC_TR, ST)*10 + reg_vreadf(tr, RTC_TR, SU);
        tm->tm_min = reg_vreadf(tr, RTC_TR, MNT)*10 + reg_vreadf(tr, RTC_TR, MNU);
        tm->tm_hour = reg_vreadf(tr, RTC_TR, HT)*10 + reg_vreadf(tr, RTC_TR, HU);
        tm->tm_mday = reg_vreadf(dr, RTC_DR, DT)*10 + reg_vreadf(dr, RTC_DR, DU);
        tm->tm_mon = reg_vreadf(dr, RTC_DR, MT)*10 + reg_vreadf(dr, RTC_DR, MU) - 1;
        tm->tm_year = 100 + reg_vreadf(dr, RTC_DR, YT)*10 + reg_vreadf(dr, RTC_DR, YU);
        tm->tm_isdst = reg_readf(RTC_CR, BKP);
    }
    else
    {
        tm->tm_sec = 0;
        tm->tm_min = 0;
        tm->tm_hour = 0;
        tm->tm_mday = 1;
        tm->tm_mon = 0;
        tm->tm_year = 101;
        tm->tm_isdst = 0;
    }

    set_day_of_week(tm);
    set_day_of_year(tm);

    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    /* Allow RTC write protection */
    reg_writef(PWR_CR1, DBP(1));

    /* Unlock registers */
    reg_writef(RTC_WPR, KEY_V(KEY1));
    reg_writef(RTC_WPR, KEY_V(KEY2));

    /* Enter initialization mode */
    reg_writef(RTC_ISR, INIT(1));
    while (!reg_readf(RTC_ISR, INITF));

    /* Program calendar and dividers */
    reg_writef(RTC_PRER, PREDIV_A(127), PREDIV_S(255));
    reg_writef(RTC_TR,
               PM(0),
               HT(tm->tm_hour / 10),
               HU(tm->tm_hour % 10),
               MNT(tm->tm_min / 10),
               MNU(tm->tm_min % 10),
               ST(tm->tm_sec / 10),
               SU(tm->tm_sec % 10));
    reg_writef(RTC_DR,
               WDU(tm->tm_wday == 0 ? 7 : tm->tm_wday),
               YT((tm->tm_year / 10) % 10),
               YU(tm->tm_year % 10),
               MT((tm->tm_mon + 1) / 10),
               MU((tm->tm_mon + 1) % 10),
               DT(tm->tm_mday / 10),
               DU(tm->tm_mday % 10));
    reg_writef(RTC_CR,
               FMT(0),
               BKP(!!tm->tm_isdst));

    /* Exit initialization */
    reg_writef(RTC_ISR, INIT(0));

    /* Lock registers */
    reg_writef(RTC_WPR, KEY(0));
    reg_writef(PWR_CR1, DBP(0));

    return 1;
}

#ifdef HAVE_RTC_ALARM
/* TODO */
void rtc_set_alarm(int h, int m)
{
    (void)h;
    (void)m;
}

void rtc_get_alarm(int *h, int *m)
{
    *h = 0;
    *m = 0;
}

void rtc_enable_alarm(bool enable)
{
    (void)enable;
}

bool rtc_check_alarm_started(bool release_alarm)
{
    (void)release_alarm;
    return false;
}

bool rtc_check_alarm_flag(void)
{
    return false;
}
#endif
