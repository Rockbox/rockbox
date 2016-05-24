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

#include "config.h"
#include "time.h"
#include "system.h"
#include "rtc.h"
#include "timefuncs.h"
#include "rtc-imx233.h"

#define YEAR1980    315532800   /* 1980/1/1 00:00:00 in UTC */

#if defined(SANSA_FUZEPLUS) || defined(CREATIVE_ZENXFI3) || defined(SONY_NWZE360) || \
    defined(SONY_NWZE370)
#define USE_PERSISTENT
#endif

void rtc_init(void)
{
    /* rtc-imx233 is initialized by the system */
}

static void seconds_to_datetime(uint32_t seconds, struct tm *tm)
{
#ifdef USE_PERSISTENT
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary. */
    seconds += imx233_rtc_read_persistent(2);
#else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    seconds += YEAR1980;
#endif

    gmtime_r(&seconds, tm);
}

int rtc_read_datetime(struct tm *tm)
{
    seconds_to_datetime(imx233_rtc_read_seconds(), tm);
    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    uint32_t seconds;

    seconds = mktime((struct tm *)tm);

#ifdef USE_PERSISTENT
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary.
     * NOTE: the OF uses this mechanism to prevent roll back in time. Although
     *       Rockbox will handle a negative PERSISTENT2 value, the OF will detect
     *       it and won't return in time before SECONDS */
    imx233_rtc_write_persistent(2, seconds - imx233_rtc_read_seconds());
#else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    imx233_rtc_write_seconds(seconds - YEAR1980);
#endif

    return 0;
}

void rtc_set_alarm(int h, int m)
{
    /* transform alarm time to absolute time */
    struct tm tm;
    seconds_to_datetime(imx233_rtc_read_seconds(), &tm);
    /* if same date and hour/min is in the past, advance one day */
    if(h < tm.tm_hour || (h == tm.tm_hour && m <= tm.tm_min))
        seconds_to_datetime(imx233_rtc_read_seconds() + 3600 * 60, &tm);
    tm.tm_hour = h;
    tm.tm_min = m;
    tm.tm_sec = 0;

    uint32_t seconds = mktime(&tm);
#ifdef USE_PERSISTENT
    imx233_rtc_write_alarm(seconds - imx233_rtc_read_persistent(2));
#else
    imx233_rtc_write_alarm(seconds - YEAR1980);
#endif
}

void rtc_get_alarm(int *h, int *m)
{
    struct tm tm;
    seconds_to_datetime(imx233_rtc_read_alarm(), &tm);
    *m = tm.tm_min;
    *h = tm.tm_hour;
}

void rtc_enable_alarm(bool enable)
{
    BF_CLR(RTC_CTRL, ALARM_IRQ_EN);
    BF_CLR(RTC_CTRL, ALARM_IRQ);
    uint32_t val = imx233_rtc_read_persistent(0);
    BF_WRX(val, RTC_PERSISTENT0, ALARM_EN(enable));
    BF_WRX(val, RTC_PERSISTENT0, ALARM_WAKE_EN(enable));
    BF_WRX(val, RTC_PERSISTENT0, ALARM_WAKE(0));
    imx233_rtc_write_persistent(0, val);
}

/**
 * Check if alarm caused unit to start.
 */
bool rtc_check_alarm_started(bool release_alarm)
{
    bool res = BF_RDX(imx233_rtc_read_persistent(0), RTC_PERSISTENT0, ALARM_WAKE);
    if(release_alarm)
        rtc_enable_alarm(false);
    return res;
}

/**
 * Checks if an alarm interrupt has triggered since last we checked.
 */
bool rtc_check_alarm_flag(void)
{
    return BF_RD(RTC_CTRL, ALARM_IRQ);
}
