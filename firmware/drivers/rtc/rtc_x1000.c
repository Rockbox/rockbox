/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm* tm)
{
    (void)tm;
    return 0;
}

int rtc_write_datetime(const struct tm* tm)
{
    (void)tm;
    return 0;
}

#ifdef HAVE_RTC_ALARM
void rtc_set_alarm(int h, int m)
{
    (void)h;
    (void)m;
}

void rtc_get_alarm(int* h, int* m)
{
    (void)h;
    (void)m;
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
