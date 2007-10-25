/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: rtc_as3514.c 12131 2007-01-27 20:48:48Z dan_a $
 *
 * Copyright (C) 2007 by Barry Wardell
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "rtc.h"
#include "i2c-pp.h"
#include "as3514.h"

#define MINUTE_SECONDS      60
#define HOUR_SECONDS        3600
#define DAY_SECONDS         86400
#define WEEK_SECONDS        604800
#define YEAR_SECONDS        31536000
#define LEAP_YEAR_SECONDS   31622400

#define BCD2DEC(X)          (((((X)>>4) & 0x0f) * 10) + ((X) & 0xf))
#define DEC2BCD(X)          ((((X)/10)<<4) | ((X)%10))

/* Days in each month */
static unsigned int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline bool is_leapyear(int year)
{
    if( ((year%4)==0) && (((year%100)!=0) || ((year%400)==0)) )
        return true;
    else
        return false;
}

void rtc_init(void)
{
}

int rtc_read_datetime(unsigned char* buf)
{
    char tmp[4];
    int year;
    int i;
    unsigned int seconds;
    
    /* RTC_AS3514's slave address is 0x46*/
    for (i=0;i<4;i++){
        tmp[i] = i2c_readbyte(AS3514_I2C_ADDR, RTC_0 + i);
    }
    seconds = tmp[0] + (tmp[1]<<8) + (tmp[2]<<16) + (tmp[3]<<24);
    
    /* Convert seconds since Jan-1-1980 to format compatible with
     * get_time() from firmware/common/timefuncs.c */
    
    /* weekday */
    buf[3] = ((seconds % WEEK_SECONDS) / DAY_SECONDS + 2) % 7;
    
    /* Year */
    year = 1980;
    while(seconds>=LEAP_YEAR_SECONDS)
    {
        if(is_leapyear(year)){
            seconds -= LEAP_YEAR_SECONDS;
        } else {
            seconds -= YEAR_SECONDS;
        }

        year++;
    }
    
    if(is_leapyear(year)) {
        days_in_month[1] = 29;
    } else {
        days_in_month[1] = 28;
        if(seconds>YEAR_SECONDS){
            year++;
            seconds -= YEAR_SECONDS;
        }
    }
    buf[6] = year%100;
    
    /* Month */
    for(i=0; i<12; i++)
    {
        if(seconds < days_in_month[i]*DAY_SECONDS){
            buf[5] = i+1;
            break;
        }
        
        seconds -= days_in_month[i]*DAY_SECONDS;
    }
    
    /* Month Day */
    buf[4] = seconds/DAY_SECONDS;
    seconds -= buf[4]*DAY_SECONDS;
    buf[4]++; /* 1 ... 31 */

    /* Hour */
    buf[2] = seconds/HOUR_SECONDS;
    seconds -= buf[2]*HOUR_SECONDS;
    
    /* Minute */
    buf[1] = seconds/MINUTE_SECONDS;
    seconds -= buf[1]*MINUTE_SECONDS;
    
    /* Second */
    buf[0] = seconds;
    
    /* Convert to Binary Coded Decimal format */
    for(i=0; i<7; i++)
        buf[i] = DEC2BCD(buf[i]);
    
    return 7;
}

int rtc_write_datetime(unsigned char* buf)
{
    int i, year;
    unsigned int year_days = 0;
    unsigned int month_days = 0;
    unsigned int seconds = 0;
    
    /* Convert from Binary Coded Decimal format */
    for(i=0; i<7; i++)
        buf[i] = BCD2DEC(buf[i]);   

    year = 2000 + buf[6];

    if(is_leapyear(year)) {
        days_in_month[1] = 29;
    } else {
        days_in_month[1] = 28;
    }
    
    /* Number of days in months gone by this year*/
    for(i=0; i<(buf[5]-1); i++){
        month_days += days_in_month[i];
    }
    
    /* Number of days in years gone by since 1-Jan-1980 */
    year_days = 365*(buf[6]+20) + (buf[6]-1)/4 + 6;

    /* Convert to seconds since 1-Jan-1980 */
    seconds = buf[0]
            + buf[1]*MINUTE_SECONDS
            + buf[2]*HOUR_SECONDS
            + (buf[4]-1)*DAY_SECONDS
            + month_days*DAY_SECONDS
            + year_days*DAY_SECONDS;

    /* Send data to RTC */
    for (i=0;i<4;i++){
        pp_i2c_send(AS3514_I2C_ADDR, RTC_0 + i, ((seconds >> (8 * i)) & 0xff));
    }
    return 1;
}

