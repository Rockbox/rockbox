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

void rtc_init(void)
{
    /* rtc-imx233 is initialized by the system */
}

int rtc_read_datetime(struct tm *tm)
{
    uint32_t seconds = imx233_rtc_read_seconds();
#if defined(SANSA_FUZEPLUS) || defined(CREATIVE_ZENXFI3)
    /* The OF uses PERSISTENT2 register to keep the adjustment and only changes
     * SECONDS if necessary. */
    seconds += imx233_rtc_read_persistent(2);
#else
    /* The Freescale recommended way of keeping time is the number of seconds
     * since 00:00 1/1/1980 */
    seconds += YEAR1980;
#endif

    gmtime_r(&seconds, tm);

    return 0;
}

int rtc_write_datetime(const struct tm *tm)
{
    uint32_t seconds;

    seconds = mktime((struct tm *)tm);

#if defined(SANSA_FUZEPLUS) || defined(CREATIVE_ZENXFI3)
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
    (void) h;
    (void) m;
}

void rtc_get_alarm(int *h, int *m)
{
    (void) h;
    (void) m;
}

void rtc_enable_alarm(bool enable)
{
    (void) enable;
}

bool rtc_check_alarm_started(bool release_alarm)
{
    (void) release_alarm;
    return false;
}

bool rtc_check_alarm_flag(void)
{
    return false;
}
