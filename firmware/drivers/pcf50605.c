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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "pcf5060x.h"
#include "pcf50605.h"

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
    pcf50605_write(PCF5060X_OOCC1,
                   GOSTDBY | CHGWAK | EXTONWAK | pcf50605_wakeup_flags);
}

void pcf50605_init(void)
{
#if (defined (IPOD_VIDEO) || defined (IPOD_NANO))
    /* I/O and GPO voltage supply. ECO not allowed regarding data sheet. Defaults:
     * iPod Video = 0xf8 = 3.3V ON
     * iPod nano  = 0xf5 = 3.0V ON */
    pcf50605_write(PCF5060X_IOREGC,  0xf5); /* 3.0V ON */
    
    /* Core voltage supply. ECO not stable, assumed due to less precision of 
     * voltage in ECO mode. DCDC2 is not relevant as this may be used for 
     * voltage scaling. Default is 1.2V ON for PP5022/PP5024 */
    pcf50605_write(PCF5060X_DCDC1,   0xec); /* 1.2V ON */
    pcf50605_write(PCF5060X_DCDC2,   0x0c); /* OFF */
    
    /* Unknown. Defaults:
     * iPod Video = 0xe3 = 1.8V ON
     * iPod nano  = 0xe3 = 1.8V ON */
    pcf50605_write(PCF5060X_DCUDC1,  0xe3); /* 1.8V ON */
    
    /* Codec voltage supply. ECO not allowed as max. current of 5mA is not
     * sufficient. Defaults:
     * iPod Video = 0xf5 = 3.0V ON
     * iPod nano  = 0xef = 2.4V ON */
    pcf50605_write(PCF5060X_D1REGC1, 0xf0); /* 2.5V ON */
    
    /* PCF5060X_D2REGC1 is set in accordance to the accessory power setting */
    
#if  defined (IPOD_VIDEO)
    /* LCD voltage supply. Defaults:
     * iPod Video = 0xf5 = 3.0V ON */
    pcf50605_write(PCF5060X_D3REGC1, 0xf1); /* 2.6V ON */
#elif defined (IPOD_NANO)
    /* D3REGC has effect on LCD and ATA, leave it unchanged due to possible ATA
     * failures. Defaults:
     * iPod nano  = 0xf5 = 3.0V ON */
    pcf50605_write(PCF5060X_D3REGC1, 0xf5); /* 3.0V ON */
#endif
    
    /* PCF5060X_LPREGC1 is leaved untouched as the setting varies over the 
     * different iPod platforms. Defaults:
     * iPod Video = 0x1f = 0ff
     * iPod nano  = 0xf6 = 3.1V ON */
#else
    /* keep initialization from svn for other iPods */
    pcf50605_write(PCF5060X_D1REGC1, 0xf5); /* 3.0V ON */
    pcf50605_write(PCF5060X_D3REGC1, 0xf5); /* 3.0V ON */
#endif
}
