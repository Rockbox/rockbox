/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Robert Kukla 
 * based on Archos code by Linus Nielsen Feltzing, Uwe Freese, Laurent Baum   
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "rtc.h" 
#include "logf.h"
#include "sw_i2c.h"

#define RTC_ADDR   0xD0

void rtc_init(void)
{
    sw_i2c_init();

#ifdef HAVE_RTC_ALARM
    /* Check + save alarm bit first, before the power thread starts watching */
    rtc_check_alarm_started(false);
#endif

}

#ifdef HAVE_RTC_ALARM    

/* check whether the unit has been started by the RTC alarm function */
/* (check for A2F, which => started using wakeup alarm) */
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
 * Checks the A2F flag.  This call resets A2F once read.
 *
 */
bool rtc_check_alarm_flag(void) 
{
    unsigned char buf[1];
    bool flag = false;

    sw_i2c_read(RTC_ADDR, 0x0f, buf, 1);        
    if (buf[0] & 0x02) flag = true;
      
    buf[0] = 0x00;
    sw_i2c_write(RTC_ADDR, 0x0f, buf, 1);        
    
    return flag;
}

/* set alarm time registers to the given time (repeat once per day) */
void rtc_set_alarm(int h, int m)
{
    unsigned char buf[3];

    buf[0] = (((m / 10) << 4) | (m % 10)) & 0x7f; /* minutes */
    buf[1] = (((h / 10) << 4) | (h % 10)) & 0x3f; /* hour */
    buf[2] = 0x80;                                /* repeat every day */
    
    sw_i2c_write(RTC_ADDR, 0x0b, buf, 3);        
}

/* read out the current alarm time */
void rtc_get_alarm(int *h, int *m)
{
    unsigned char buf[2];
     
    sw_i2c_read(RTC_ADDR, 0x0b, buf, 2);        

    *m = ((buf[0] & 0x70) >> 4) * 10 + (buf[0] & 0x0f);
    *h = ((buf[1] & 0x30) >> 4) * 10 + (buf[1] & 0x0f);
}

/* turn alarm on or off by setting the alarm flag enable */
/* the alarm is automatically disabled when the RTC gets Vcc power at startup */
/* avoid that an alarm occurs when the device is on because this locks the ON key forever */
/* returns false if alarm was set and alarm flag (output) is off */
/* returns true if alarm flag went on, which would lock the device, so the alarm was disabled again */
bool rtc_enable_alarm(bool enable)
{
    unsigned char buf[2];
    
    buf[0] = enable ? 0x26 : 0x24; /* BBSQI INTCN A2IE vs INTCH  only */
    buf[1] = 0x00; /* reset alarm flags (and OSF for good measure) */
    
    sw_i2c_write(RTC_ADDR, 0x0e, buf, 2);        

    return false; /* all ok */
}

#endif /* HAVE_RTC_ALARM */ 

int rtc_read_datetime(unsigned char* buf)
{
    int i;

    i = sw_i2c_read(RTC_ADDR, 0, buf, 7);        
    
    buf[3]--; /* timefuncs wants 0..6 for wday */

    return i;
}

int rtc_write_datetime(unsigned char* buf)
{
    int i;

    buf[3]++;       /* chip wants 1..7 for wday */
    buf[5]|=0x80;   /* chip wants century (always 20xx) */

    i = sw_i2c_write(RTC_ADDR, 0, buf, 7);

    return i;
}

