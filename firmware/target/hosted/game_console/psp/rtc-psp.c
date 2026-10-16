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
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"

#include <psprtc.h>

void rtc_init(void)
{
    /* nothing to do */
}

int rtc_read_datetime(struct tm *tm)
{
    ScePspDateTime info;
    sceRtcGetCurrentClockLocalTime(&info);

    memset(tm, 0, sizeof(struct tm));
    tm->tm_year = info.year - 1900;
    tm->tm_mon = info.month - 1;
    tm->tm_hour = info.hour;
    tm->tm_min = info.minute;
    tm->tm_sec = info.second;
    tm->tm_wday = sceRtcGetDayOfWeek(info.year, info.month, info.day);
    tm->tm_mday = info.day;

    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    /* cannot set date and time from pspsdk */
    (void)(*tm);
    return 0;
}
