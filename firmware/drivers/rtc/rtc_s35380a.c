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

/* REALTIME_DATA register bytes */
#define TIME_YEAR     0
#define TIME_MONTH    1
#define TIME_DAY      2
#define TIME_WEEKDAY  3
#define TIME_HOUR     4
#define TIME_MINUTE   5
#define TIME_SECOND   6
#define TIME_REG_SIZE 7

/* INT1, INT2 register bytes */
#define ALARM_WEEKDAY  0
#define ALARM_HOUR     1
#define ALARM_MINUTE   2
#define ALARM_REG_SIZE 3

/* INT1, INT2 register bits */
#define A1WE 0x80
#define A1HE 0x80
#define A1mE 0x80

#define A2WE 0x80
#define A2HE 0x80
#define A2mE 0x80

#define AMPM 0x40

static bool int_flag;

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

static inline void rtc_reset(void)
{
    unsigned char reg = STATUS_REG1_RESET;
    i2c_write(I2C_IFACE_1, RTC_ADDR|(STATUS_REG1<<1), &reg, 1);
}

void rtc_init(void)
{
    unsigned char reg;
    static bool initialized = false;

    if ( initialized )
        return;

    i2c_read(I2C_IFACE_1, RTC_ADDR|(STATUS_REG1<<1), &reg, 1);

    /* cache INT1, INT2 flags as reading the register seem to clear
     * this bits (which is not described in datasheet)
     */
    int_flag = ((reg & STATUS_REG1_INT1) || (reg & STATUS_REG1_INT2));

    /* test POC and BLD flags */
    if ( (reg & STATUS_REG1_POC) || (reg & STATUS_REG1_BLD))
        rtc_reset();

    i2c_read(I2C_IFACE_1, RTC_ADDR|(STATUS_REG2<<1), &reg, 1);

    /* test TEST flag */
    if ( reg & STATUS_REG2_TEST )
        rtc_reset();

    /* setup 24h time format */
    reg = STATUS_REG1_H1224;
    i2c_write(I2C_IFACE_1, RTC_ADDR|(STATUS_REG1<<1), &reg, 1);

    initialized = true;
}

int rtc_read_datetime(struct tm *tm)
{
    unsigned char buf[TIME_REG_SIZE];
    unsigned int i;
    int ret;

    ret = i2c_read(I2C_IFACE_1, RTC_ADDR|(REALTIME_DATA1<<1), buf, sizeof(buf));
    reverse_bits(buf, sizeof(buf));

    buf[TIME_HOUR] &= 0x3f; /* mask out p.m. flag */

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = BCD2DEC(buf[i]);

    tm->tm_sec  = buf[TIME_SECOND];
    tm->tm_min  = buf[TIME_MINUTE];
    tm->tm_hour = buf[TIME_HOUR];
    tm->tm_wday = buf[TIME_WEEKDAY];
    tm->tm_mday = buf[TIME_DAY];
    tm->tm_mon  = buf[TIME_MONTH] - 1;
    tm->tm_year = buf[TIME_YEAR] + 100;

    return ret;
}

int rtc_write_datetime(const struct tm *tm)
{
    unsigned char buf[TIME_REG_SIZE];
    unsigned int i;
    int ret;

    buf[TIME_SECOND]  = tm->tm_sec;
    buf[TIME_MINUTE]  = tm->tm_min;
    buf[TIME_HOUR]    = tm->tm_hour;
    buf[TIME_WEEKDAY] = tm->tm_wday;
    buf[TIME_DAY]     = tm->tm_mday;
    buf[TIME_MONTH]   = tm->tm_mon + 1;
    buf[TIME_YEAR]    = tm->tm_year - 100;

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = DEC2BCD(buf[i]);

    reverse_bits(buf, sizeof(buf));
    ret = i2c_write(I2C_IFACE_1, RTC_ADDR|(REALTIME_DATA1<<1), buf, sizeof(buf));

    return ret;
}

#ifdef HAVE_RTC_ALARM
void rtc_set_alarm(int h, int m)
{
    unsigned char buf[ALARM_REG_SIZE];

    /* INT1 register can be accessed only when IN1AE flag is set */
    rtc_enable_alarm(true);

    /* A1mE, A1HE - validity flags */
    buf[ALARM_MINUTE]  = DEC2BCD(m) | A1mE;
    buf[ALARM_HOUR]    = DEC2BCD(h) | A1HE;
    buf[ALARM_WEEKDAY] = 0;

    /* AM/PM flag have to be set properly regardles of
     * time format used (H1224 flag in STATUS_REG1)
     * this is not described in datasheet for s35380a
     * but is somehow described in datasheet for s35390a
     */
    if ( h >= 12 )
        buf[ALARM_HOUR] |= AMPM;

    reverse_bits(buf, sizeof(buf));
    i2c_write(I2C_IFACE_1, RTC_ADDR|(INT1_REG<<1), buf, sizeof(buf));
}

void rtc_get_alarm(int *h, int *m)
{
    unsigned char buf[ALARM_REG_SIZE];

    /* INT1 alarm register can be accessed only when INT1AE is set */
    rtc_enable_alarm(true);

    /* read the content of INT1 register */
    i2c_read(I2C_IFACE_1, RTC_ADDR|(INT1_REG<<1), buf, sizeof(buf));
    reverse_bits(buf, sizeof(buf));

    *h = BCD2DEC(buf[ALARM_HOUR] & 0x3f); /* mask out A1HE and PM/AM flag */
    *m = BCD2DEC(buf[ALARM_MINUTE] & 0x7f); /* mask out A1mE */

    rtc_enable_alarm(false);
}

bool rtc_check_alarm_flag(void)
{
    unsigned char reg;
    i2c_read(I2C_IFACE_1, RTC_ADDR|(STATUS_REG1<<1), &reg, 1);

    return ((reg & STATUS_REG1_INT1) || (reg & STATUS_REG1_INT2));
}

void rtc_enable_alarm(bool enable)
{
    unsigned char reg = 0;

    if (enable)
        reg = STATUS_REG2_INT1AE;

    i2c_write(I2C_IFACE_1, RTC_ADDR|(STATUS_REG2<<1), &reg, 1);
}

bool rtc_check_alarm_started(bool release_alarm)
{
    static bool run_before;
    bool rc;

    if (run_before)
    {
        rc = int_flag;
        int_flag &= ~release_alarm;
    }
    else
    {
        rc = int_flag;
        run_before = true;
    }

    return rc;
}
#endif
