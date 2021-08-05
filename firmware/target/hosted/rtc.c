/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Based upon code (C) 2002 by Björn Stenberg
 * Copyright (C) 2011 by Thomas Jarosch
 * Copyright (C) 2018 by Marcin Bukat
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
#include <time.h>
#include <sys/time.h>
#if !defined(WIN32)
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#endif

#include "config.h"

void rtc_init(void)
{
  tzset();
}

int rtc_read_datetime(struct tm *tm)
{
    time_t now = time(NULL);
    *tm = *localtime(&now);

    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
#if !defined(WIN32)
    struct timeval tv;
    struct tm *tm_time;

    tv.tv_sec = mktime((struct tm *)tm);
    tv.tv_usec = 0;

    if ((int)tv.tv_sec == -1)
        return -1;

    /* set system clock */
    settimeofday(&tv, NULL);

    /* hw clock stores time in UTC */
    time_t now = time(NULL);
    tm_time = gmtime(&now);

    /* Try to write the HW RTC, if present. */
    int rtc = open("/dev/rtc0", O_WRONLY | O_CLOEXEC);
    if (rtc > 0) {
        ioctl(rtc, RTC_SET_TIME, (struct rtc_time *)tm_time);
        close(rtc);
    }
#else
    (void)(*tm);
#endif
    return 0;
}

#if defined(HAVE_RTC_ALARM) && !defined(SIMULATOR)
void rtc_set_alarm(int h, int m)
{
    struct rtc_time tm;
    long sec;

    int rtc = open("/dev/rtc0", O_WRONLY | O_CLOEXEC);
    if (rtc < 0)
        return;

    /* Get RTC time */
    ioctl(rtc, RTC_RD_TIME, &tm);

    /* Convert to seconds into the GMT day.  Can be negative! */
    sec = h * 3600 + m * 60 + timezone;
    h = sec / 3600;
    sec -= h * 3600;
    m = sec / 60;

    /* Handle negative or positive wraps */
    while (m < 0) {
        m += 60;
        h--;
    }
    while (m > 59) {
        m -= 60;
        h++;
    }
    while (h < 0) {
        h += 24;
	tm.tm_mday--;
    }
    while (h > 23) {
        h -= 24;
	tm.tm_mday++;
    }

    /* Update the struct */
    tm.tm_sec = 0;
    tm.tm_hour = h;
    tm.tm_min = m;

    ioctl(rtc, RTC_ALM_SET, &tm);
    close(rtc);
}

void rtc_get_alarm(int *h, int *m)
{
    struct rtc_time tm;
    long sec;

    int rtc = open("/dev/rtc0", O_WRONLY | O_CLOEXEC);
    if (rtc < 0)
        return;

    ioctl(rtc, RTC_ALM_READ, &tm);
    close(rtc);

    /* Convert RTC from UTC to local time zone.. */
    sec = (tm.tm_min * 60) + (tm.tm_hour * 3600) - timezone;

    /* Handle wrapping and negative offsets */
    *h = (sec / 3600);
    sec -= *h * 3600;
    *m = sec / 60;

    while (*m < 0) {
        *m = *m + 60;
        *h = *h - 1;
    }
    while (*m > 59) {
        *m = *m - 60;
        *h = *h + 1;
    }
    while (*h < 0) {
        *h = *h + 24;
    }
    while (*h > 23) {
        *h = *h - 24;
    }
}

void rtc_enable_alarm(bool enable)
{
    int rtc = open("/dev/rtc0", O_WRONLY | O_CLOEXEC);
    if (rtc < 0)
        return;

    ioctl(rtc, enable ? RTC_AIE_ON : RTC_AIE_OFF, NULL);
    close(rtc);

    /* XXX Note that this may or may not work; Linux may need to be suspended
       or shut down in a special way to keep the RTC alarm active */
}

/* Returns true if alarm was the reason we started up */
bool rtc_check_alarm_started(bool release_alarm)
{
    int rtc = open("/dev/rtc0", O_WRONLY | O_CLOEXEC);
    if (rtc < 0)
        return false;

    /* XXX There is no generic way of determining wakeup reason.  Will
       likely need a target-specific hook. */

    /* Disable alarm if requested */
    if (release_alarm)
        ioctl(rtc, RTC_AIE_OFF, NULL);

    close(rtc);
    return false;
}

/* See if we received an alarm. */
bool rtc_check_alarm_flag(void)
{
    struct rtc_wkalrm alrm;

    int rtc = open("/dev/rtc0", O_WRONLY | O_CLOEXEC);
    if (rtc < 0)
        return false;

    alrm.pending = 0;
    /* XXX Documented as "mostly useless on Linux" except with EFI RTCs
       Will likely need a target-specific hook. */
    ioctl(rtc, RTC_WKALM_RD, &alrm);
    close(rtc);

    return alrm.pending;
}

#endif /* HAVE_RTC_ALARM */
