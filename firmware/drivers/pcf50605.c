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
#include "system.h"
#include "config.h"
#if CONFIG_I2C == I2C_PP5020 || CONFIG_I2C == I2C_PP5002
#include "i2c-pp.h"
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
#define DCDC1   0x1b
#define DCDC2   0x1c
#define DCDC3   0x1d
#define DCDC4   0x1e
#define DCDEC1  0x1f
#define DCDEC2  0x20
#define DCUDC1  0x21
#define DCUDC2  0x22
#define IOREGC  0x23
#define D1REGC1 0x24
#define D2REGC1 0x25
#define D3REGC1 0x26
#define LPREG1C 0x27


unsigned char pcf50605_wakeup_flags = 0;

int pcf50605_read(int address)
{
    return i2c_readbyte(0x8,address);
}

int pcf50605_read_multiple(int address, unsigned char* buf, int count)
{
    int read = i2c_readbytes(0x08, address, count, buf);
    return read - count;
}

int pcf50605_write(int address, unsigned char val)
{
    pp_i2c_send(0x8, address, val);
    return 0;
}

int pcf50605_write_multiple(int address, const unsigned char* buf, int count)
{
    int i;

    i2c_lock();

    for (i = 0; i < count; i++)
        pp_i2c_send(0x8, address + i, buf[i]);

    i2c_unlock();

    return 0;
}

/* The following command puts the iPod into a deep sleep.  Warning
   from the good people of ipodlinux - never issue this command
   without setting CHGWAK or EXTONWAK if you ever want to be able to
   power on your iPod again. */
void pcf50605_standby_mode(void)
{
    pcf50605_write(OOCC1, GOSTDBY | CHGWAK | EXTONWAK | pcf50605_wakeup_flags);
}

void pcf50605_init(void)
{
#if defined (IPOD_VIDEO)    
    /* I/O and GPO voltage supply (default: 0xf8 = 3.3V ON) */
    /* ECO not allowed regarding data sheet */
    pcf50605_write(IOREGC,  0xf8);  /* 3.3V ON */
    
    /* core voltage supply (default DCDC1/DCDC2: 0xec = 1.2V ON) */
    /* ECO not stable, assumed due to less precision of voltage in ECO mode */
    pcf50605_write(DCDC1,   0xec);  /* 1.2V ON */
    pcf50605_write(DCDC2,   0x0c);  /* OFF */
    
    /* unknown (default: 0xe3 = 1.8V ON) */
    pcf50605_write(DCUDC1,  0xe3);  /* 1.8V ON */
    
    /* WM8758 voltage supply (default: 0xf5 = 3.0V ON) */
    /* ECO not allowed as max. current of 5mA is not sufficient */
    pcf50605_write(D1REGC1, 0xf0);  /* 2.5V ON */
    
    /* LCD voltage supply (default: 0xf5 = 3.0V ON) */
    pcf50605_write(D3REGC1, 0xf1);  /* 2.6V ON */
#else
    /* keep initialization from svn for other iPods */
    pcf50605_write(D1REGC1, 0xf5); /* 3.0V ON */
    pcf50605_write(D3REGC1, 0xf5); /* 3.0V ON */
#endif
    /* Dock Connector pin 17 (default: OFF) */
    pcf50605_write(D2REGC1, 0xf8); /* 3.3V ON */
}
