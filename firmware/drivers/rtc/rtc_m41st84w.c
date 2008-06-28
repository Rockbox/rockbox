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
#include <stdbool.h>

#define RTC_ADR 0xd0
#define	RTC_DEV_WRITE   (RTC_ADR | 0x00)
#define	RTC_DEV_READ    (RTC_ADR | 0x01)

void rtc_init(void)
{
    unsigned char data; 

#ifdef HAVE_RTC_ALARM
    /* Check + save alarm bit first, before the power thread starts watching */
    rtc_check_alarm_started(false);
#endif

    rtc_write(0x13, 0x10); /* 32 kHz square wave */

    /* Clear the Stop bit if it is set */
    data = rtc_read(0x01);
    if(data & 0x80)
        rtc_write(0x01, 0x00);

    /* Clear the HT bit if it is set */
    data = rtc_read(0x0c);

    if(data & 0x40)
    {
        data &= ~0x40;
        rtc_write(0x0c,data);
    }
    
#ifdef HAVE_RTC_ALARM    

    /* Clear Trec bit, write-protecting the RTC for 200ms when shutting off */
    /* without this, the alarm won't work! */
    
    data = rtc_read(0x04);
    if (data & 0x80)
    {
        data &= ~0x80;
        rtc_write(0x04, data);
    }

    /* Also, make sure that the OUT bit in register 8 is 1,
       otherwise the player can't be turned off. */
    rtc_write(8, rtc_read(8) | 0x80);
    
#endif
}

#ifdef HAVE_RTC_ALARM    

/* check whether the unit has been started by the RTC alarm function */
/* (check for AF, which => started using wakeup alarm) */
bool rtc_check_alarm_started(bool release_alarm)
{
    static bool alarm_state, run_before;
    bool rc;

    if (run_before) {
        rc = alarm_state;
        alarm_state &= ~release_alarm;
    } else { 
        /* This call resets AF, so we store the state for later recall */ 
        rc = alarm_state = rtc_check_alarm_flag();
        run_before = true; 
    }
 
    return rc;
}
/*
 * Checks the AL register.  This call resets AL once read.
 *
 * We're only interested if ABE is set.  AL is still raised regardless
 * even if the unit is off when the alarm occurs.
 */
bool rtc_check_alarm_flag(void) 
{
    return ( ( (rtc_read(0x0f) & 0x40) != 0) &&
               (rtc_read(0x0a) & 0x20)  );
}

/* set alarm time registers to the given time (repeat once per day) */
void rtc_set_alarm(int h, int m)
{
    unsigned char data;

    /* for daily alarm, RPT5=RPT4=on, RPT1=RPT2=RPT3=off */
    
    rtc_write(0x0e, 0x00);                       /* seconds 0 and RTP1 */
    rtc_write(0x0d, ((m / 10) << 4) | (m % 10)); /* minutes and RPT2 */
    rtc_write(0x0c, ((h / 10) << 4) | (h % 10)); /* hour and RPT3 */
    rtc_write(0x0b, 0xc1);                       /* set date 01 and RPT4 and RTP5 */
    
    /* set month to 1, if it's invalid, the rtc does an alarm every second instead */
    data = rtc_read(0x0a);
    data &= 0xe0;
    data |= 0x01;
    rtc_write(0x0a, data);
}

/* read out the current alarm time */
void rtc_get_alarm(int *h, int *m)
{
    unsigned char data;

    data = rtc_read(0x0c);
    *h = ((data & 0x30) >> 4) * 10 + (data & 0x0f);
    
    data = rtc_read(0x0d);
    *m = ((data & 0x70) >> 4) * 10 + (data & 0x0f);
}

/* turn alarm on or off by setting the alarm flag enable */
/* the alarm is automatically disabled when the RTC gets Vcc power at startup */
/* avoid that an alarm occurs when the device is on because this locks the ON key forever */
/* returns false if alarm was set and alarm flag (output) is off */
/* returns true if alarm flag went on, which would lock the device, so the alarm was disabled again */
bool rtc_enable_alarm(bool enable)
{
    unsigned char data = rtc_read(0x0a);
    if (enable)
    {
        data |= 0xa0; /* turn bit d7=AFE and d5=ABE on */
    }
    else
        data &= 0x5f; /* turn bit d7=AFE and d5=ABE off */
    rtc_write(0x0a, data);
    
    /* check if alarm flag AF is off (as it should be) */
    /* in some cases enabling the alarm results in an activated AF flag */
    /* this should not happen, but it does */
    /* if you know why, tell me! */
    /* for now, we try again forever in this case */
    while (rtc_check_alarm_flag()) /* on */
    {
        data &= 0x5f; /* turn bit d7=AFE and d5=ABE off */
        rtc_write(0x0a, data);
	sleep(HZ / 10);
	rtc_check_alarm_flag();
        data |= 0xa0; /* turn bit d7=AFE and d5=ABE on */
        rtc_write(0x0a, data);
    }

    return false; /* all ok */
}

#endif /* HAVE_RTC_ALARM */

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

    /* send read command */
    if (i2c_write(RTC_DEV_READ,buf,1) >= 0)
    {
        i2c_start();
        i2c_outb(RTC_DEV_READ);
        if (i2c_getack())
        {
            value = i2c_inb(1);
        }
    }

    i2c_stop();

    i2c_end();
    return value;
}

int rtc_read_multiple(unsigned char address, unsigned char *buf, int numbytes)
{
    int ret = 0;
    unsigned char obuf[1];
    int i;

    i2c_begin();
    
    obuf[0] = address;

    /* send read command */
    if (i2c_write(RTC_DEV_READ, obuf, 1) >= 0)
    {
        i2c_start();
        i2c_outb(RTC_DEV_READ);
        if (i2c_getack())
        {
            for(i = 0;i < numbytes-1;i++)
                buf[i] = i2c_inb(0);
            
            buf[i] = i2c_inb(1);
        }
        else
        {
            ret = -1;
        }
    }

    i2c_stop();

    i2c_end();
    return ret;
}

int rtc_read_datetime(unsigned char* buf) {
    int rc;

    rc = rtc_read_multiple(1, buf, 7);

    /* Adjust weekday */
    if(buf[3] == 7)
        buf[3]=0;

    return rc;
}

int rtc_write_datetime(unsigned char* buf) {
    int i;
    int rc = 0;
    
    /* Adjust weekday */
    if(buf[3] == 0)
        buf[3] = 7;

    for (i = 0; i < 7 ; i++)
    {
        rc |= rtc_write(i+1, buf[i]);
    }
    rc |= rtc_write(8, 0x80); /* Out=1, calibration = 0 */

    return rc;
}
