/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for pcf50605 PMU and RTC
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/pcf50605.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#if CONFIG_I2C == I2C_PP5020
#include "i2c-pp5020.h"
#elif CONFIG_I2C == I2C_PP5002
#include "i2c-pp5002.h"
#endif
#include "rtc.h"

#define OOCS  0x01
#define INT1  0x02
#define INT2  0x03
#define INT3  0x04
#define INT1M 0x05
#define INT2M 0x06
#define INT3M 0x07
#define OOCC1 0x08
  #define GOSTDBY  0x1
  #define TOTRST   (0x1 << 1)
  #define CLK32ON  (0x1 << 2)
  #define WDTRST   (0x1 << 3)
  #define RTCWAK   (0x1 << 4)
  #define CHGWAK   (0x1 << 5)
  #define EXTONWAK (0x01 << 6)
#define OOCC2 0x09
#define RTCSC 0x0a
#define RTCMN 0x0b
#define RTCHR 0x0c
#define RTCWD 0x0d
#define RTCDT 0x0e
#define RTCMT 0x0f
#define RTCYR 0x10
#define RTCSCA 0x11
#define RTCMNA 0x12
#define RTCHRA 0x13
#define RTCWDA 0x14
#define RTCDTA 0x15
#define RTCMTA 0x16
#define RTCYRA 0x17
#define PSSC   0x18
#define PWROKM 0x19
#define PWROKS 0x1a

int pcf50605_read(int address)
{
    return i2c_readbyte(0x8,address);
}

int pcf50605_read_multiple(int address, unsigned char* buf, int count)
{
    int i;

    for (i=0;i<count;i++)
    {
        buf[i]=pcf50605_read(address);
        address++;
    }

    return 0;
}

int pcf50605_write(int address, unsigned char val)
{
    ipod_i2c_send(0x8, address, val);
    return 0;
}

int pcf50605_write_multiple(int address, const unsigned char* buf, int count)
{
    /* TODO */
    (void)address;
    (void)buf;
    (void)count;
    return 0;
}

unsigned short pcf50605_a2d_read(int adc_input)
{
    unsigned short hi;
    unsigned char lo;

    ipod_i2c_send(0x8, 0x2f, (adc_input<<1) | 0x1); /* ADCC2, ADCMUX = adc_input, ADCSTART = 1 */
    hi = i2c_readbyte(0x8, 0x30);           /* ADCS1 */
    lo = (i2c_readbyte(0x8, 0x31) & 0x3);   /* ADCS2 */

    return (hi << 2) | lo;
}

/* The following command puts the iPod into a deep sleep.  Warning
   from the good people of ipodlinux - never issue this command
   without setting CHGWAK or EXTONWAK if you ever want to be able to
   power on your iPod again. */
void pcf50605_standby_mode(void)
{
    pcf50605_write(OOCC1, GOSTDBY | CHGWAK | EXTONWAK);
}

