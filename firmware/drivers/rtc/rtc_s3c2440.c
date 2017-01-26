/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing, Uwe Freese, Laurent Baum
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
#include "i2c.h"
#include "rtc.h"
#include "kernel.h"
#include "system.h"
#include "timefuncs.h"

void rtc_init(void)
{
    /* RTC Enable */
    RTCCON |= 1;
}

int rtc_read_datetime(struct tm *tm)
{
    tm->tm_sec = BCD2DEC(BCDSEC);
    tm->tm_min = BCD2DEC(BCDMIN);
    tm->tm_hour = BCD2DEC(BCDHOUR);
    tm->tm_mday = BCD2DEC(BCDDATE);
    tm->tm_mon = BCD2DEC(BCDMON) - 1;
    tm->tm_year = BCD2DEC(BCDYEAR) + 100;
    tm->tm_yday = 0; /* Not implemented for now */

    set_day_of_week(tm);

    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    BCDSEC  = DEC2BCD(tm->tm_sec);
    BCDMIN  = DEC2BCD(tm->tm_min);
    BCDHOUR = DEC2BCD(tm->tm_hour);
    BCDDAY  = DEC2BCD(tm->tm_wday) + 1; /* chip wants 1..7 for wday */
    BCDDATE = DEC2BCD(tm->tm_mday);
    BCDMON  = DEC2BCD(tm->tm_mon + 1);
    BCDYEAR = DEC2BCD(tm->tm_year - 100);

    return 1;
}

#ifdef HAVE_RTC_ALARM
/* This alarm code works with a flashed bootloader.  This will not work with
 * the OF bootloader.
 */
 
/* Check whether the unit has been started by the RTC alarm function */
bool rtc_check_alarm_started(bool release_alarm)
{
    if (GSTATUS3) 
    {
        GSTATUS3 &= ~release_alarm;  
        return true;
    } 
    else 
    { 
        return false;
    }
}

/* Check to see if the alarm has flaged since the last it was checked */
bool rtc_check_alarm_flag(void) 
{
    bool ret=SRCPND & 0x40000000;
    
    SRCPND=RTC_MASK;
    
    return ret;
}

/* set alarm time registers to the given time (repeat once per day) */
void rtc_set_alarm(int h, int m)
{
    ALMMIN = DEC2BCD(m); /* minutes */
    ALMHOUR = DEC2BCD(h); /* hour */
}

/* read out the current alarm time */
void rtc_get_alarm(int *h, int *m)
{
    *m = BCD2DEC(ALMMIN);
    *h = BCD2DEC(ALMHOUR);
}

/* turn alarm on or off by setting the alarm flag enable
 */
void rtc_enable_alarm(bool enable)
{
    if (enable)
    {
        RTCALM=0x46;
    }
    else
    {
        RTCALM=0x00;
    }
}
#endif
