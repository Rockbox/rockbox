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

#if CONFIG_RTC

/* Common functions for all targets */
void rtc_init(void);
int rtc_read_datetime(unsigned char* buf);
int rtc_write_datetime(unsigned char* buf);

#if CONFIG_RTC == RTC_M41ST84W

/* The RTC in the Archos devices is used for much more than just the clock 
   data */
int rtc_read(unsigned char address);
int rtc_read_multiple(unsigned char address, unsigned char *buf, int numbytes);
int rtc_write(unsigned char address, unsigned char value);

#endif /* RTC_M41ST84W */

#ifdef HAVE_RTC_ALARM
void rtc_set_alarm(int h, int m);
void rtc_get_alarm(int *h, int *m);
bool rtc_enable_alarm(bool enable);
bool rtc_check_alarm_started(bool release_alarm);
bool rtc_check_alarm_flag(void);
#endif /* HAVE_RTC_ALARM */

#endif /* CONFIG_RTC */

#endif
