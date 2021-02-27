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
 * Based mainly on rtc_jz4760.c,
 *   Copyright (C) 2016 by Roman Stolyarov
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
#include "x1000/rtc.h"
#include <stdint.h>

/* 4 byte magic number 'RTCV' */
#define RTCV 0x52544356

/* expected RTC clock frequency */
#define NC1HZ_EXPECTED (32768 - 1)

static void rtc_write_reg(uint32_t addr, uint32_t value)
{
    while(jz_readf(RTC_CR, WRDY) == 0);
    REG_RTC_WENR = 0xa55a;
    while(jz_readf(RTC_WENR, WEN) == 0);
    while(jz_readf(RTC_CR, WRDY) == 0);
    (*(volatile uint32_t*)addr) = value;
    while(jz_readf(RTC_CR, WRDY) == 0);
}

void rtc_init(void)
{
    /* Check if we totally lost power and need to reset the RTC */
    if(REG_RTC_HSPR != RTCV || jz_readf(RTC_GR, NC1HZ) != NC1HZ_EXPECTED) {
        rtc_write_reg(JA_RTC_GR, NC1HZ_EXPECTED);
        rtc_write_reg(JA_RTC_HWFCR, 3200);
        rtc_write_reg(JA_RTC_HRCR, 2048);
        rtc_write_reg(JA_RTC_SR, 1546300800); /* 01/01/2019 */
        rtc_write_reg(JA_RTC_CR, 1);
        rtc_write_reg(JA_RTC_HSPR, RTCV);
    }

    rtc_write_reg(JA_RTC_HWRSR, 0);
}

int rtc_read_datetime(struct tm* tm)
{
    time_t time = REG_RTC_SR;
    gmtime_r(&time, tm);
    return 1;
}

int rtc_write_datetime(const struct tm* tm)
{
    time_t time = mktime((struct tm*)tm);
    rtc_write_reg(JA_RTC_SR, time);
    return 1;
}

#ifdef HAVE_RTC_ALARM
/* TODO: implement the RTC alarm */

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
