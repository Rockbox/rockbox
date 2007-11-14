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
#include "i2c-pp.h"
#include <stdbool.h>

/* RTC registers */
#define RTC_CTRL1   0x00
#define RTC_CTRL2   0x01

/* Control 2 register flags */
#define RTC_TF  0x04
#define RTC_AF  0x08

void rtc_init(void)
{
    unsigned char tmp;
    int rv;
    
    /* initialize Control 1 register */
    tmp = 0;
    pp_i2c_send(0x51, RTC_CTRL1,tmp);
    
    /* read value of the Control 2 register - we'll need it to preserve alarm and timer interrupt assertion flags */
    rv = i2c_readbytes(0x51,RTC_CTRL2,1,&tmp);
    /* clear alarm and timer interrupts */
    tmp &= (RTC_TF | RTC_AF);
    pp_i2c_send(0x51, RTC_CTRL2,tmp);
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

