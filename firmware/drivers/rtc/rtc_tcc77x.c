/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Dave Chapman
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
#include "rtc.h"
#include "system.h"
#include <stdbool.h>

void rtc_init(void)
{
}

int rtc_read_datetime(unsigned char* buf)
{

}

int rtc_write_datetime(unsigned char* buf)
{
    return 1;
}

/**
 * Checks to see if an alarm interrupt has triggered since last we checked.
 */
bool rtc_check_alarm_flag(void) 
{

}

/**
 * Enables or disables the alarm.
 */
bool rtc_enable_alarm(bool enable)
{
}

/**
 * Check if alarm caused unit to start.
 */
bool rtc_check_alarm_started(bool release_alarm)
{
}

void rtc_set_alarm(int h, int m)
{
    /* Convert to BCD */
//    pcf50605_write(0x12, ((m/10) << 4) | m%10);
//    pcf50605_write(0x13, ((h/10) << 4) | h%10);
}

void rtc_get_alarm(int *h, int *m)
{
    char buf[2];

    /* Convert from BCD */
//    *m = ((buf[0] >> 4) & 0x7)*10 + (buf[0] & 0x0f);
//    *h = ((buf[1] >> 4) & 0x3)*10 + (buf[1] & 0x0f);
}

