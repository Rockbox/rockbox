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

#include "config.h"

#include "rtc.h"
#include "time.h"
#include "debug.h"

static struct tm tm;

struct tm *get_time(void)
{
    char rtcbuf[8];
    int i;

    /* We don't need the first byte, but we want the indexes in the
       buffer to match the data sheet */
    rtc_read_multiple(1, &rtcbuf[1], 7);

    for(i = 0;i < 7;i++)
    {
        DEBUGF("%02x ", rtcbuf[i]);
    }
    DEBUGF("\n");
    
    tm.tm_sec = ((rtcbuf[1] & 0x70) >> 4) * 10 + (rtcbuf[1] & 0x0f);
    tm.tm_min = ((rtcbuf[2] & 0x70) >> 4) * 10 + (rtcbuf[2] & 0x0f);
    tm.tm_hour = ((rtcbuf[3] & 0x30) >> 4) * 10 + (rtcbuf[3] & 0x0f);
    tm.tm_mday = ((rtcbuf[5] & 0x30) >> 4) * 10 + (rtcbuf[5] & 0x0f);
    tm.tm_mon = ((rtcbuf[6] & 0x10) >> 4) * 10 + (rtcbuf[6] & 0x0f);
    tm.tm_year = ((rtcbuf[7] & 0xf0) >> 4) * 10 + (rtcbuf[7] & 0x0f) + 2000;
    tm.tm_wday = rtcbuf[4] & 0x07;
    tm.tm_yday = 0; /* Not implemented for now */
    tm.tm_isdst = -1; /* Not implemented for now */
    return &tm;
}
