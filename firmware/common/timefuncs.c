/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h> /* get NULL */
#include "config.h"

#include "kernel.h"
#include "rtc.h"
#include "timefuncs.h"
#include "debug.h"

#ifndef SIMULATOR
static struct tm tm;
#endif

bool valid_time(const struct tm *tm)
{
    if (tm->tm_hour < 0 || tm->tm_hour > 23 ||
        tm->tm_sec < 0 || tm->tm_sec > 59 || 
        tm->tm_min < 0 || tm->tm_min > 59 ||
        tm->tm_year < 100 || tm->tm_year > 199 ||
        tm->tm_mon < 0 || tm->tm_mon > 11 ||
        tm->tm_wday < 0 || tm->tm_wday > 6 ||
        tm->tm_mday < 1 || tm->tm_mday > 31)
        return false;
    else
        return true;
}

static int last_tick = 0;

struct tm *get_time(void)
{
#ifndef SIMULATOR
#ifdef CONFIG_RTC

    /* Don't read the RTC more than 4 times per second */
    if (last_tick + HZ/4 < current_tick) {
        char rtcbuf[7];
        rtc_read_datetime(rtcbuf);

        tm.tm_sec = ((rtcbuf[0] & 0x70) >> 4) * 10 + (rtcbuf[0] & 0x0f);
        tm.tm_min = ((rtcbuf[1] & 0x70) >> 4) * 10 + (rtcbuf[1] & 0x0f);
        tm.tm_hour = ((rtcbuf[2] & 0x30) >> 4) * 10 + (rtcbuf[2] & 0x0f);
        tm.tm_wday = rtcbuf[3] & 0x07;
        tm.tm_mday = ((rtcbuf[4] & 0x30) >> 4) * 10 + (rtcbuf[4] & 0x0f);
        tm.tm_mon = ((rtcbuf[5] & 0x10) >> 4) * 10 + (rtcbuf[5] & 0x0f) - 1;
        tm.tm_year = ((rtcbuf[6] & 0xf0) >> 4) * 10 + (rtcbuf[6] & 0x0f) + 100;

        tm.tm_yday = 0; /* Not implemented for now */
        tm.tm_isdst = -1; /* Not implemented for now */
    }
#else
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    tm.tm_mday = 1;
    tm.tm_mon = 0;
    tm.tm_year = 70;
    tm.tm_wday = 1;
    tm.tm_yday = 0; /* Not implemented for now */
    tm.tm_isdst = -1; /* Not implemented for now */
#endif
    return &tm;
#else
    time_t now = time(NULL);
    return localtime(&now);
#endif
}

int set_time(const struct tm *tm)
{
#ifdef CONFIG_RTC
    int rc;
    char rtcbuf[7];
    
    if (valid_time(tm))
    {
        rtcbuf[0]=((tm->tm_sec/10) << 4) | (tm->tm_sec%10);
        rtcbuf[1]=((tm->tm_min/10) << 4) | (tm->tm_min%10);
        rtcbuf[2]=((tm->tm_hour/10) << 4) | (tm->tm_hour%10);
        rtcbuf[3]=tm->tm_wday;
        rtcbuf[4]=((tm->tm_mday/10) << 4) | (tm->tm_mday%10);
        rtcbuf[5]=(((tm->tm_mon+1)/10) << 4) | ((tm->tm_mon+1)%10);
        rtcbuf[6]=(((tm->tm_year-100)/10) << 4) | ((tm->tm_year-100)%10);

        rc = rtc_write_datetime(rtcbuf);

        if(rc)
            return -1;
        else
            return 0;
    }
    else
    {
        return -2;
    }
#else
    (void)tm;
    return 0;
#endif
}
