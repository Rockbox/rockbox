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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _RTC_H_
#define _RTC_H_

#include <stdbool.h> 

#ifdef HAVE_RTC
void rtc_init(void);
int rtc_read(unsigned char address);
int rtc_read_multiple(unsigned char address, unsigned char *buf, int numbytes);
int rtc_write(unsigned char address, unsigned char value);

#ifdef HAVE_ALARM_MOD  
void rtc_set_alarm(int h, int m);
void rtc_get_alarm(int *h, int *m);
bool rtc_enable_alarm(bool enable);
bool rtc_check_alarm_started(bool release_alarm);
#endif /* HAVE_ALARM_MOD */

#endif /* HAVE_RTC */

#endif
