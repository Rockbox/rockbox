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
#ifdef HAVE_RTC
#include "i2c.h"
#include "rtc.h"

#define RTC_ADR 0xd0
#define	RTC_DEV_WRITE   (RTC_ADR | 0x00)
#define	RTC_DEV_READ    (RTC_ADR | 0x01)

int rtc_write(unsigned char address, unsigned char value)
{
    int ret = 0;
    unsigned char buf[2];

    i2c_begin();
    
    buf[0] = address;
    buf[1] = value;

    /* send run command */
    if (i2c_write(RTC_DEV_WRITE,buf,2))
    {
        ret = -1;
    }

    i2c_end();
    return ret;
}


int rtc_read(unsigned char address)
{
    int value = -1;
    unsigned char buf[1];

    i2c_begin();
    
    buf[0] = address;

    /* send run command */
    if (i2c_write(RTC_DEV_READ,buf,1) >= 0)
    {
        i2c_start();
        i2c_outb(RTC_DEV_READ);
        if (i2c_getack())
        {
            value = i2c_inb(1);
        }
    }

    i2c_end();
    return value;
}

#endif
