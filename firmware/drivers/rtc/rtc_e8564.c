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
#include "i2c-pp5020.h"
#include <stdbool.h>

void rtc_init(void)
{
}

int rtc_read_datetime(unsigned char* buf)
{
    unsigned char tmp;
    int read;
    
    /*RTC_E8564's slave address is 0x51*/
    read = i2c_readbytes(0x51,0x02,7,buf);

    /* swap wday and mday to be compatible with
     * get_time() from firmware/common/timefuncs.c */
    tmp=buf[3];
    buf[3]=buf[4];
    buf[4]=tmp;
    
    return read;
}

int rtc_write_datetime(unsigned char* buf)
{
    int i;
    unsigned char tmp;
    
    /* swap wday and mday to be compatible with
     * set_time() in firmware/common/timefuncs.c */
    tmp=buf[3];
    buf[3]=buf[4];
    buf[4]=tmp;

    for (i=0;i<7;i++){
        pp_i2c_send(0x51, 0x02+i,buf[i]);
    }
    return 1;
}

