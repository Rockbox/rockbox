/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Barry Wardell
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
#include <stdbool.h>
#include "config.h"
#include "rtc.h"
#include "as3514.h"
#include "ascodec.h"
#include "time.h"

/* AMS Sansas start counting from Jan 1st 1970 instead of 1980 (not as3525v2) */
#if (CONFIG_CPU==AS3525)
#define SECS_ADJUST 0           /* 1970-1-1 00:00:00 */
#elif (CONFIG_CPU==AS3525v2)
#if defined(SANSA_CLIPZIP)
#define SECS_ADJUST 0           /* 1970-1-1 00:00:00 */
#else
#define SECS_ADJUST ((2*365*24*3600) + 26*(24*3600) - 7*3600 - 25*60)
#endif
#else
#define SECS_ADJUST 315532800   /* 1980-1-1 00:00:00 */
#endif

#ifdef HAVE_RTC_ALARM /* as3543 */
static struct {
    unsigned int seconds;   /* total seconds to wakeup */
    int hour;               /* wake-up hour */
    int min;                /* wake-up minute */
    bool enabled;           /* alarm enabled or not */
    unsigned char flag;     /* flag used by the OF */
} alarm;

void rtc_set_alarm(int h, int m)
{
    alarm.hour = h;
    alarm.min = m;
}

void rtc_get_alarm(int *h, int *m)
{
    *h = alarm.hour;
    *m = alarm.min;
}

/* Reads the AS3543 wakeup register */
static void alarm_read(void)
{
    unsigned char buf[6];
    unsigned int i;
    int oldstatus;

    /* read raw data */
    oldstatus = disable_irq_save();
    ascodec_read(0);
    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = ascodec_read(AS3543_WAKEUP);
    }
    restore_irq(oldstatus);
    
    /* decode */
    alarm.seconds = buf[0] | (buf[1] << 8) | ((buf[2] & 0x7F) << 16);
    alarm.enabled = buf[2] & (1 << 7);
    alarm.flag = buf[3];
    alarm.hour = buf[4];
    alarm.min = buf[5];
}

/* Writes the AS3543 wakeup register */
static void alarm_write(void)
{
    unsigned char buf[6];
    unsigned int i;
    int oldstatus;
    
    /* encode */
    buf[0] = alarm.seconds  & 0xFF;
    buf[1] = (alarm.seconds >> 8) & 0xFF;
    buf[2] = ((alarm.seconds >> 16) & 0x7F) | (alarm.enabled ? (1 << 7) : 0);
    buf[3] = alarm.flag;
    buf[4] = alarm.hour;
    buf[5] = alarm.min;
    
    /* write raw data */
    oldstatus = disable_irq_save();
    ascodec_read(0);
    for (i = 0; i < sizeof(buf); i++) {
        ascodec_write(AS3543_WAKEUP, buf[i]);
    }
    restore_irq(oldstatus);
}

void rtc_alarm_poweroff(void)
{
    if(!alarm.enabled)
        return;

    struct tm tm;
    rtc_read_datetime(&tm);
    int hours = alarm.hour - tm.tm_hour;
    int mins  = alarm.min - tm.tm_min;
    if(mins < 0)
    {
        mins += 60;
        hours -= 1;
    }
    if(hours < 0)
        hours += 24;

    uint32_t seconds = hours*3600 + mins*60;
    if(seconds == 0)
        seconds = 24*3600;

    seconds -= tm.tm_sec;
#ifndef SAMSUNG_YPR0
    /* disable MCLK, it is a wakeup source and prevents proper shutdown */
    CGU_AUDIO = (2 << 0) | (1 << 11);
    CGU_PLLBSUP = (1 << 2) | (1 << 3);
#endif
    /* write wakeup register */
    alarm.seconds = seconds;
    alarm.enabled = true;
    alarm_write();

    /* enable heartbeat watchdog */
    ascodec_write(AS3514_SYSTEM, (1<<3) | (1<<0));

    /* In_Cntr : disable heartbeat source */
    ascodec_write_pmu(0x1a, 4, ascodec_read_pmu(0x1a, 4) & ~(3<<2));

    while(1);
}

void rtc_enable_alarm(bool enable)
{
    alarm.enabled = enable;
}

bool rtc_check_alarm_started(bool release_alarm)
{
    /* read wakeup register and check if alarm was enabled */
    alarm_read();
    if (!alarm.enabled) {
        return false;
    }

    alarm.enabled = !release_alarm;
    alarm_write();

    struct tm tm;
    rtc_read_datetime(&tm);

    /* were we powered up at the programmed time ? */
    return alarm.hour == tm.tm_hour && alarm.min == tm.tm_min;
}

bool rtc_check_alarm_flag(void)
{
    /* We don't need to do anything special if it has already fired */
    return false;
}
#endif /* HAVE_RTC_ALARM */

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    char tmp[4];
    unsigned int seconds;
    int i;

    /* Get seconds time stamp from RTC */
    for (i = 0; i < 4; i++){
        tmp[i] = ascodec_read(AS3514_RTC_0 + i);
    }
    seconds = tmp[0] + (tmp[1]<<8) + (tmp[2]<<16) + (tmp[3]<<24);

    /* convert to struct tm */
    time_t time = seconds + SECS_ADJUST;
    gmtime_r(&time, tm);

    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    time_t time;
    unsigned int seconds;
    int i;
    
    /* convert struct tm to time stamp */
    time = mktime((struct tm *)tm);
    seconds = time - SECS_ADJUST;

    /* Send data to RTC */
    for (i=0; i<4; i++){
        ascodec_write(AS3514_RTC_0 + i, ((seconds >> (8 * i)) & 0xff));
    }
    return 1;
}
