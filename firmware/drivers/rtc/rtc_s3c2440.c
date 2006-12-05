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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
#include <stdbool.h>

void rtc_init(void)
{
    /* RTC Enable */
    RTCCON |= 1;
}

int rtc_read_datetime(unsigned char* buf)
{
    buf[0] = BCDSEC;
    buf[1] = BCDMIN;
    buf[2] = BCDHOUR;
    buf[3] = BCDDAY;
    buf[4] = BCDDATE;
    buf[5] = BCDMON;
    buf[6] = BCDYEAR;

    return 1;
}

int rtc_write_datetime(unsigned char* buf)
{
    BCDSEC  = buf[0];
    BCDMIN  = buf[1];
    BCDHOUR = buf[2];
    BCDDAY  = buf[3];
    BCDDATE = buf[4];
    BCDMON  = buf[5];
    BCDYEAR = buf[6];

    return 1;
}

