/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing, Uwe Freese
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
#ifndef _RTC_H_
#define _RTC_H_

#include <stdbool.h> 
#include "system.h"
#include "config.h"
#include "time.h"

/* Macros used to convert to and from BCD, used in various rtc drivers
   this is the wrong place but misc.h is in apps... */
#define BCD2DEC(X) (((((X)>>4) & 0x0f) * 10) + ((X) & 0xf))
#define DEC2BCD(X) ((((X)/10)<<4) | ((X)%10))

#if CONFIG_RTC

/* Common functions for all targets */
void rtc_init(void);
int rtc_read_datetime(struct tm *tm);
int rtc_write_datetime(const struct tm *tm);

#ifdef HAVE_RTC_ALARM
void rtc_set_alarm(int h, int m);
void rtc_get_alarm(int *h, int *m);
void rtc_enable_alarm(bool enable);
bool rtc_check_alarm_started(bool release_alarm);
bool rtc_check_alarm_flag(void);
#endif /* HAVE_RTC_ALARM */

#endif /* CONFIG_RTC */

#endif

