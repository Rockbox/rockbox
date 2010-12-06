/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * adopted for HD300 by Marcin Bukat
 * Copyright (C) 2009 by Bertrik Sikken
 * Copyright (C) 2008 by Robert Kukla 
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
#include "rtc.h" 
#include "i2c-coldfire.h"

#include "lcd.h"
#include "font.h"
/*  Driver for the Seiko S35380A real-time clock chip with i2c interface

    This driver was derived from rtc_s3539a.c and adapted for the MPIO HD300
 */

#define RTC_ADDR   0x60

#define STATUS_REG1     0
#define STATUS_REG2     1
#define REALTIME_DATA1  2
#define REALTIME_DATA2  3
#define INT1_REG        4
#define INT2_REG        5
#define CLOCK_CORR_REG  6
#define FREE_REG        7

/* STATUS_REG1 flags
 * bits order is reversed
 */
#define STATUS_REG1_POC   0x01
#define STATUS_REG1_BLD   0x02
#define STATUS_REG1_INT2  0x04
#define STATUS_REG1_INT1  0x08
#define STATUS_REG1_SC1   0x10
#define STATUS_REG1_SC0   0x20
#define STATUS_REG1_H1224 0x40
#define STATUS_REG1_RESET 0x80

/* STATUS_REG2 flags
 * bits order is reversed
 */
#define STATUS_REG2_TEST   0x01
#define STATUS_REG2_INT2AE 0x02
#define STATUS_REG2_INT2ME 0x04
#define STATUS_REG2_INT2FE 0x08
#define STATUS_REG2_32kE   0x10
#define STATUS_REG2_INT1AE 0x20
#define STATUS_REG2_INT1ME 0x40
#define STATUS_REG2_INT1FE 0x80

static void reverse_bits(unsigned char* v, int size)
{
    static const unsigned char flipnibble[] =
        {0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
         0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F};
    int i;
                                            
    for (i = 0; i < size; i++) {
        v[i] = (flipnibble[v[i] & 0x0F] << 4) |
                flipnibble[(v[i] >> 4) & 0x0F];
    }
}    

void rtc_init(void)
{
    unsigned char status_reg;
    i2c_read(I2C_IFACE_1, RTC_ADDR | (STATUS_REG1<<1), &status_reg, 1);

    if ( (status_reg & STATUS_REG1_POC) ||
         (status_reg & STATUS_REG1_BLD) )
    {
        /* perform rtc reset*/
        status_reg |= STATUS_REG1_RESET;
        i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG1<<1), &status_reg, 1);
    }

    /* setup 24h time format */
    status_reg = STATUS_REG1_H1224;
    i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG1<<1), &status_reg, 1);

#ifdef HAVE_RTC_ALARM
    rtc_check_alarm_started(false);
#endif
    /* disable all alarms */
    status_reg = 0;
    i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG2<<1), &status_reg, 1);
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned char buf[7];
    unsigned int i;
    int ret;

    ret = i2c_read(I2C_IFACE_1, RTC_ADDR | (REALTIME_DATA1<<1), buf, sizeof(buf));
    reverse_bits(buf, sizeof(buf));

    buf[4] &= 0x3f; /* mask out p.m. flag */

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = BCD2DEC(buf[i]);

    tm->tm_sec = buf[6];
    tm->tm_min = buf[5];
    tm->tm_hour = buf[4];
    tm->tm_wday = buf[3];
    tm->tm_mday = buf[2];
    tm->tm_mon = buf[1] - 1;
    tm->tm_year = buf[0] + 100;

    return ret;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned char buf[7];
    unsigned int i;
    int ret;

    buf[6] = tm->tm_sec;
    buf[5] = tm->tm_min;
    buf[4] = tm->tm_hour;
    buf[3] = tm->tm_wday;
    buf[2] = tm->tm_mday;
    buf[1] = tm->tm_mon + 1;
    buf[0] = tm->tm_year - 100;

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = DEC2BCD(buf[i]);

    reverse_bits(buf, sizeof(buf));
    ret = i2c_write(I2C_IFACE_1, RTC_ADDR | (REALTIME_DATA1<<1), buf, sizeof(buf));

    return ret;
}

#ifdef HAVE_RTC_ALARM
void rtc_set_alarm(int h, int m)
{
    /* 1) get the date
     * 2) compare h:m with current time
          if alarm time < current time set day += 1
       3) check day if it is not needed to wrap around
       4) set the validity bits
       5) write to alarm register
     */

    unsigned char buf[3];
    unsigned char reg;

    /* 0x80 - validity flag */
    buf[2] = DEC2BCD(m) | 0x80;
    buf[1] = DEC2BCD(h) | 0x80;
    buf[0] = 0;

    reverse_bits(buf, sizeof(buf));

    reg = STATUS_REG2_INT1AE;
    i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG2<<1), &reg, 1);
    i2c_write(I2C_IFACE_1, RTC_ADDR | (INT1_REG<<1), buf, sizeof(buf));
}

void rtc_get_alarm(int *h, int *m)
{
    unsigned char buf[3];
    unsigned char reg,reg2;

    i2c_read(I2C_IFACE_1, RTC_ADDR | (STATUS_REG2<<1), &reg, 1);

    reg2 = reg | STATUS_REG2_INT1AE;
    i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG2<<1), &reg2, 1);
    i2c_read(I2C_IFACE_1, RTC_ADDR | (INT1_REG<<1), buf, sizeof(buf));
    i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG2<<1), &reg, 1);

    reverse_bits(buf, sizeof(buf));

    *h = BCD2DEC(buf[1] & 0x3f); /* mask out A1HE and PM/AM flag */
    *m = BCD2DEC(buf[2] & 0x7f); /* mask out A1mE */    
}

bool rtc_check_alarm_flag(void)
{
    unsigned char status_reg;
    i2c_read(I2C_IFACE_1, RTC_ADDR | (STATUS_REG1<<1), &status_reg, 1);

    return (status_reg & (STATUS_REG1_INT1 | STATUS_REG1_INT2));
}

void rtc_enable_alarm(bool enable)
{
    unsigned char status_reg2;
    status_reg2 = enable ? STATUS_REG2_INT1AE:0;
    i2c_write(I2C_IFACE_1, RTC_ADDR | (STATUS_REG2<<1), &status_reg2, 1);
}

bool rtc_check_alarm_started(bool release_alarm)
{
    static bool run_before = false, alarm_state;
    bool rc;

    if (run_before)
    {
        rc = alarm_state;
        alarm_state &= ~release_alarm;
    }
    else
    {
        rc = rtc_check_alarm_flag();
        run_before = true;
    }

    return rc;
}
#endif
