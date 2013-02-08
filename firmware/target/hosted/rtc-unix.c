/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Lorenzo Miori
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

#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h> /* atexit */

/** 
 * A generic standard RTC device wrapper. Necessary on hosted platforms
 * where the RTC is the default /dev/rtc device and a low-level access is preferable
 * and nicer e.g. hybrid environments (Samsung YP-R0 / YP-R1), without relying on system time
 * TODO
 *      - Add Alarm Clock feature, where available
 */

/* Some targets may have special things to care about i.e. differences with respect to OF */
#ifdef SAMSUNG_YPR1
#define SECS_ADJUST     3600
/* Define this below if you care about system time */
#define UPDATE_SYS_TIME
#endif

void rtc_close(void);
void rtc_open(void);
#ifdef UPDATE_SYS_TIME
void rtc_update(void);
#endif

static int rtc_dev = -1;

void rtc_init(void)
{
    rtc_update();
    atexit(rtc_update);
}

void rtc_open(void) {
    if (rtc_dev < 0) {
        rtc_dev = open("/dev/rtc", O_RDONLY);
    }
}

void rtc_close(void)
{
    if (rtc_dev > 0) {
        close(rtc_dev);
        rtc_dev = -1;
    }
}

#ifdef UPDATE_SYS_TIME
void rtc_update(void) {
    system("hwclock -s");
}
#endif

int rtc_read_datetime(struct tm *tm)
{
    rtc_open();
    int ret = ioctl(rtc_dev, RTC_RD_TIME, tm);
#ifdef SECS_ADJUST
    tm->tm_sec += SECS_ADJUST;
    mktime((struct tm *)tm);
#endif
    rtc_close();
    return ret;
}

int rtc_write_datetime(const struct tm *tm)
{
    int ret;
    rtc_open();
#ifdef SECS_ADJUST
    struct tm time_to_set = *tm;
    time_to_set.tm_sec -= SECS_ADJUST;
    mktime(&time_to_set);
    ret = ioctl(rtc_dev, RTC_SET_TIME, &time_to_set);
#else
    ret = ioctl(rtc_dev, RTC_SET_TIME, tm);
#endif
    rtc_close();
    /* Workaround to read RTC time back to system. May fail, we don't care
     * This is needed in YP-R1, for example, where a sistematic hwclock -w
     * is called before shutting down the system. A constrain is that /dev/rtc shall
     * be closed before use hwclock, or it won't work at all
     */
#ifdef UPDATE_SYS_TIME
    rtc_update();
#endif

    return ret;
}
