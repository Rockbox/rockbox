/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
 *
 * Based on code from the iPodLinux project (C) 2004-2005 Bernard Leach
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "i2c-pp5020.h"
#include "rtc.h"

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
    /* TODO */
    (void)address;
    (void)val;
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

static int pcf50605_a2d_read(int adc_input)
{
    int hi, lo;

    /* Enable ACD module */
    ipod_i2c_send(0x8, 0x33, 0x80); /* ACDC1, ACDAPE = 1 */

    /* select ADCIN1 - subtractor, and start sampling process */
    ipod_i2c_send(0x8, 0x2f, (adc_input<<1) | 0x1); /* ADCC2, ADCMUX = adc_input, ADCSTART = 1 */

    /* ADCC2, wait for ADCSTART = 0 (wait for sampling to start) */
    while ((i2c_readbyte(0x8, 0x2f) & 1)) /* do nothing */;

    /* ADCS2, wait ADCRDY = 0 (wait for sampling to end) */
    while (!(i2c_readbyte(0x8, 0x31) & 0x80)) /* do nothing */;

    hi = i2c_readbyte(0x8, 0x30);           /* ADCS1 */
    lo = (i2c_readbyte(0x8, 0x31) & 0x3);   /* ADCS2 */

    return (hi << 2) | lo;
}

int pcf50605_battery_read(void)
{
    return pcf50605_a2d_read(0x3);       /* ADCIN1, subtractor */
}

void rtc_init(void)
{
    /* Nothing to do. */
}

int rtc_read_datetime(unsigned char* buf)
{
    int rc;

    rc = pcf50605_read_multiple(0x0a, buf, 7);

    return rc;
}


int rtc_write_datetime(unsigned char* buf)
{
    int i;

    for (i=0;i<7;i++) {
        ipod_i2c_send(0x8, 0x0a+i, buf[i]);
    }

    return 1;
}
