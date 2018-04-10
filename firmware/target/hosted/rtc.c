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
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <unistd.h>

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    time_t now = time(NULL);
    *tm = *localtime(&now);

    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
#if defined(AGPTEK_ROCKER)
    struct timeval tv;
    struct tm *tm_time;

    int rtc = open("/dev/rtc0", O_WRONLY);
    
    tv.tv_sec = mktime((struct tm *)tm);
    tv.tv_usec = 0;

    /* set system clock */
    settimeofday(&tv, NULL);

    /* hw clock stores time in UTC */
    time_t now = time(NULL);
    tm_time = gmtime(&now);

    ioctl(rtc, RTC_SET_TIME, (struct rtc_time *)tm_time);
    close(rtc);

    return 0;
#else
    return -1;
#endif
}
