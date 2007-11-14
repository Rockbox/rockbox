/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing, Uwe Freese, Laurent Baum, 
 *                       Przemyslaw Holubowski
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
#define RTC_CTRL1          0x00
#define RTC_CTRL2          0x01
#define RTC_ALARM_MINUTES  0x09
#define RTC_ALARM_HOURS    0x0A
#define RTC_ALARM_DAY      0x0B
#define RTC_ALARM_WDAY     0x0C
#define RTC_TIMER_CTRL     0x0E
#define RTC_TIMER          0x0F

/* Control 2 register flags */
#define RTC_TIE    0x01
#define RTC_AIE    0x02
#define RTC_TF     0x04
#define RTC_AF     0x08
#define RTC_TI_TP  0x10

/* Alarm registers flags */
#define RTC_AE     0x80

/* Timer register flags */
#define RTC_TE     0x80

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

void rtc_set_alarm(int h, int m)
{
    unsigned char buf[4]={0};
    int rv=0;
    int i=0;
    
    /* clear alarm interrupt */
    rv = i2c_readbytes(0x51,RTC_CTRL2,1,buf);
    buf[0] &= RTC_AF;
    pp_i2c_send(0x51, RTC_CTRL2,buf[0]);
    
    /* prepare new alarm */
    if( m >= 0 )
        buf[0] = (((m/10) << 4) | m%10);
    else
        /* ignore minutes comparison query */
        buf[0] = RTC_AE;
    
    if( h >= 0 )
        buf[1] = (((h/10) << 4) | h%10);
    else
        /* ignore hours comparison query */
        buf[1] = RTC_AE;
    
    /* ignore day and wday */
    buf[2] = RTC_AE;
    buf[3] = RTC_AE;
    
    /* write new alarm */
    for(;i<4;i++)
        pp_i2c_send(0x51, RTC_ALARM_MINUTES+i,buf[i]);
    
    /* note: alarm is not enabled at the point */
}

void rtc_get_alarm(int *h, int *m)
{
    unsigned char buf[4]={0};
    
    /* get alarm preset */
    i2c_readbytes(0x51, RTC_ALARM_MINUTES,4,buf);

    *m = ((buf[0] >> 4) & 0x7)*10 + (buf[0] & 0x0f);
    *h = ((buf[1] >> 4) & 0x3)*10 + (buf[1] & 0x0f);

}

bool rtc_enable_alarm(bool enable)
{
    unsigned char tmp=0;
    int rv=0;
    
    if(enable)
    {
        /* enable alarm interrupt */
        rv = i2c_readbytes(0x51,RTC_CTRL2,1,&tmp);
        tmp |= RTC_AIE;
        pp_i2c_send(0x51, RTC_CTRL2,tmp);
    }
    else
    {
        /* disable alarm interrupt */
        rv = i2c_readbytes(0x51,RTC_CTRL2,1,&tmp);
        tmp &= ~RTC_AIE;
        pp_i2c_send(0x51, RTC_CTRL2,tmp);
    }
    
    return false;
}

bool rtc_check_alarm_started(bool release_alarm)
{
    static bool alarm_state, run_before = false;
    unsigned char tmp=0;
    bool started;
    int rv=0;

    if (run_before)
    {
        started = alarm_state;
        alarm_state &= ~release_alarm;         
    } 
    else 
    { 
        /* read Control 2 register which contains alarm flag */
        rv = i2c_readbytes(0x51,RTC_CTRL2,1,&tmp);

        alarm_state = started = tmp & (RTC_AF | RTC_AIE);
        
        if(release_alarm)
        {
            rv = i2c_readbytes(0x51,RTC_CTRL2,1,&tmp);
            /* clear alarm interrupt enable and alarm flag */
            tmp &= ~(RTC_AF | RTC_AIE);
            pp_i2c_send(0x51, RTC_CTRL2,tmp);
        }
        run_before = true;
    }
    
    return started;
}

bool rtc_check_alarm_flag(void)
{
    unsigned char tmp=0;
    int rv=0;
    
    /* read Control 2 register which contains alarm flag */
    rv = i2c_readbytes(0x51,RTC_CTRL2,1,&tmp);

    return (tmp & RTC_AF);
}
