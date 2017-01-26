/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Rob Purchase
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
#include "system.h"
#include "pcf50606.h"
#include "pcf50635.h"
#include "pmu-target.h"
#include "timefuncs.h"

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned int i;
    int rc, oldlevel;
    unsigned char buf[7];

    oldlevel = disable_irq_save();

    if (get_pmu_type() == PCF50606)
        rc = pcf50606_read_multiple(PCF5060X_RTCSC, buf, sizeof(buf));
    else
        rc = pcf50635_read_multiple(PCF5063X_REG_RTCSC, buf, sizeof(buf));

    restore_irq(oldlevel);

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = BCD2DEC(buf[i]);

    tm->tm_sec = buf[0];
    tm->tm_min = buf[1];
    tm->tm_hour = buf[2];
    tm->tm_mday = buf[4];
    tm->tm_mon = buf[5] - 1;
    tm->tm_year = buf[6] + 100;
    tm->tm_yday = 0; /* Not implemented for now */

    set_day_of_week(tm);

    return rc;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned int i;
    int rc, oldlevel;
    unsigned char buf[7];

    buf[0] = tm->tm_sec;
    buf[1] = tm->tm_min;
    buf[2] = tm->tm_hour;
    buf[3] = tm->tm_wday;
    buf[4] = tm->tm_mday;
    buf[5] = tm->tm_mon + 1;
    buf[6] = tm->tm_year - 100;

    for (i = 0; i < sizeof(buf); i++)
         buf[i] = DEC2BCD(buf[i]);

    oldlevel = disable_irq_save();

    if (get_pmu_type() == PCF50606)
        rc = pcf50606_write_multiple(PCF5060X_RTCSC, buf, sizeof(buf));
    else
        rc = pcf50635_write_multiple(PCF5063X_REG_RTCSC, buf, sizeof(buf));

    restore_irq(oldlevel);

    return rc;
}
