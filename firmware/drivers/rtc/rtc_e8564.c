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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

/*RTC_E8564's slave address is 0x51*/
#define RTC_ADDR   0x51

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

bool rtc_lock_alarm_clear=true;

void rtc_init(void)
{
    unsigned char tmp;
    int rv;
    
    /* initialize Control 1 register */
    tmp = 0;
    pp_i2c_send(RTC_ADDR, RTC_CTRL1, tmp);
    
    /* read value of the Control 2 register - we'll need it to preserve alarm and timer interrupt assertion flags */
    rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, &tmp);
    /* preserve alarm and timer interrupt flags */
    tmp &= (RTC_TF | RTC_AF | RTC_TIE | RTC_AIE);
    pp_i2c_send(RTC_ADDR, RTC_CTRL2, tmp);
}

int rtc_read_datetime(struct tm *tm)
{
    int read;
    unsigned char buf[7];

    read = i2c_readbytes(RTC_ADDR, 0x02, sizeof(buf), buf);

    /* convert from bcd, avoid getting extra bits */
    tm->tm_sec  = BCD2DEC(buf[0] & 0x7f);
    tm->tm_min  = BCD2DEC(buf[1] & 0x7f);
    tm->tm_hour = BCD2DEC(buf[2] & 0x3f);
    tm->tm_mday = BCD2DEC(buf[3] & 0x3f);
    tm->tm_wday = BCD2DEC(buf[4] & 0x7);
    tm->tm_mon  = BCD2DEC(buf[5] & 0x1f) - 1;
    tm->tm_year = BCD2DEC(buf[6]) + 100;

    return read;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned int i;
    unsigned char buf[7];

    buf[0] = tm->tm_sec;
    buf[1] = tm->tm_min;
    buf[2] = tm->tm_hour;
    buf[3] = tm->tm_mday;
    buf[4] = tm->tm_wday;
    buf[5] = tm->tm_mon + 1;
    buf[6] = tm->tm_year - 100;

    for (i = 0; i < sizeof(buf); i++)
        pp_i2c_send(RTC_ADDR, 0x02 + i, DEC2BCD(buf[i]));

    return 1;
}

void rtc_set_alarm(int h, int m)
{
    unsigned char buf[4] = {0};
    int i, rv;

    /* clear alarm interrupt */
    rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, buf);
    buf[0] &= RTC_AF;
    pp_i2c_send(RTC_ADDR, RTC_CTRL2, buf[0]);

    /* prepare new alarm */
    if( m >= 0 )
        buf[0] = DEC2BCD(m);
    else
        /* ignore minutes comparison query */
        buf[0] = RTC_AE;
    
    if( h >= 0 )
        buf[1] = DEC2BCD(h);
    else
        /* ignore hours comparison query */
        buf[1] = RTC_AE;

    /* ignore day and wday */
    buf[2] = RTC_AE;
    buf[3] = RTC_AE;

    /* write new alarm */
    for(i = 0; i < 4; i++)
        pp_i2c_send(RTC_ADDR, RTC_ALARM_MINUTES + i, buf[i]);

    /* note: alarm is not enabled at the point */
}

void rtc_get_alarm(int *h, int *m)
{
    unsigned char buf[4]={0};

    /* get alarm preset */
    i2c_readbytes(RTC_ADDR, RTC_ALARM_MINUTES, 4 ,buf);

    *m = BCD2DEC(buf[0] & 0x7f);
    *h = BCD2DEC(buf[0] & 0x3f);
}

bool rtc_enable_alarm(bool enable)
{
    unsigned char tmp=0;
    int rv=0;

    if(enable)
    {
        /* enable alarm interrupt */
        rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, &tmp);
        tmp |= RTC_AIE;
        tmp &= ~RTC_AF;
        pp_i2c_send(RTC_ADDR, RTC_CTRL2, tmp);
    }
    else
    {
        /* disable alarm interrupt */       
        if(rtc_lock_alarm_clear)
            /* lock disabling alarm before it was checked whether or not the unit was started by RTC alarm */
            return false;        
        rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, &tmp);
        tmp &= ~(RTC_AIE | RTC_AF);
        pp_i2c_send(RTC_ADDR, RTC_CTRL2, tmp);
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
        rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, &tmp);

        alarm_state = started = ( (tmp & RTC_AF) && (tmp & RTC_AIE) );
        
        if(release_alarm && started)
        {
            rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, &tmp);
            /* clear alarm interrupt enable and alarm flag */
            tmp &= ~(RTC_AF | RTC_AIE);
            pp_i2c_send(RTC_ADDR, RTC_CTRL2, tmp);
        }
        run_before = true;
        rtc_lock_alarm_clear = false;
    }
    
    return started;
}

bool rtc_check_alarm_flag(void)
{
    unsigned char tmp=0;
    int rv=0;
    
    /* read Control 2 register which contains alarm flag */
    rv = i2c_readbytes(RTC_ADDR, RTC_CTRL2, 1, &tmp);

    return (tmp & RTC_AF);
}

