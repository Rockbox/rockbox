/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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
#include "i2c-s5l8700.h"

/*  Driver for the Seiko S35390A real-time clock chip with i2c interface

    This driver was derived from rtc_mr100.c and adapted for the S35390A
    used in the Meizu M3 (and possibly others).
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
}

int rtc_read_datetime(unsigned char* buf)
{
    unsigned char data[7];
    int i, ret;

    ret = i2c_read(RTC_ADDR | (REALTIME_DATA1 << 1), -1, sizeof(data), data);
    reverse_bits(data, sizeof(data));

    buf[4] &= 0x3f; /* mask out p.m. flag */
    
    for (i = 0; i < 7; i++) {
        buf[i] = data[6 - i];
    }
    
    return ret;
}

int rtc_write_datetime(unsigned char* buf)
{
    unsigned char data[7];
    int i, ret;

    for (i = 0; i < 7; i++) {
        data[i] = buf[6 - i];
    }
        
    reverse_bits(data, sizeof(data));
    ret = i2c_write(RTC_ADDR | (REALTIME_DATA1 << 1), -1, sizeof(data), data);
    
    return ret;
}

