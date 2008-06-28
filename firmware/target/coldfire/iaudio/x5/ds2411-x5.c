/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include "system.h"
#include "cpu.h"
#include "ds2411.h"
#include "logf.h"

/* Delay factor that depends on CPU frequency */
static unsigned int ds2411_delay_factor = 0;

#define DS2411_BIT (1 << 22)

/* Delay the specified number of microseconds - plus a tiny bit */
#define DELAY(uS)                \
    asm volatile(                   \
        "move.l %[x], %%d0      \n" \
        "mulu.l %[factor], %%d0 \n" \
    "1:                         \n" \
        "subq.l #1, %%d0        \n" \
        "bhi.s  1b              \n" \
        : : [factor]"d,d"(ds2411_delay_factor), [x]"r,i"(uS) : "d0");

/* Calculate the CRC of a byte */
static unsigned char ds2411_calc_crc(unsigned char byte)
{
    /* POLYNOMIAL = X^8 + X^5 + X^4 + 1 */
    static const unsigned char eor[8] =
    {
        0x5e, /* 01011110 */
        0xbc, /* 10111100 */
        0x61, /* 01100001 */
        0xc2, /* 11000010 */
        0x9d, /* 10011101 */
        0x23, /* 00100011 */
        0x46, /* 01000110 */
        0x8c, /* 10001100 */
    };

    unsigned char crc = 0;
    int i = 0;

    do
    {
        if (byte & (1 << i))
            crc ^= eor[i];
    }
    while (++i < 8);

    return crc;
} /* ds2411_calc_crc */

/* Write a byte to the DS2411 - LSb first */
static void ds2411_write_byte(unsigned char data)
{
    int i = 0;

    do
    {
        if (data & 0x01)
        {
            /* Write a "1": Hold line low, then leave line pulled up and wait
               out the remainder of Tslot */
            or_l(DS2411_BIT, &GPIO_ENABLE);
            DELAY(6);
            and_l(~DS2411_BIT, &GPIO_ENABLE);
            DELAY(60);
        }
        else
        {
            /* Write a "0": Hold line low, then leave line pulled up and wait
               out the remainder of Tslot which is just Trec */
            or_l(DS2411_BIT, &GPIO_ENABLE);
            DELAY(60);
            and_l(~DS2411_BIT, &GPIO_ENABLE);
            DELAY(6);
        }

        data >>= 1;
    }
    while (++i < 8);
} /* ds2411_write_byte */

/* Read a byte from the DS2411 - LSb first */
static unsigned char ds2411_read_byte(void)
{
    int i = 0;
    unsigned data = 0;

    do
    {
        /* Hold line low to begin bit read: Tf + Trl */
        or_l(DS2411_BIT, &GPIO_ENABLE);
        DELAY(6);

        /* Set line high and delay before sampling within the master
           sampling window: Tmsr - max 15us from Trl start */
        and_l(~DS2411_BIT, &GPIO_ENABLE);
        DELAY(9);

        /* Sample data line */
        if ((GPIO_READ & DS2411_BIT) != 0)
            data |= 1 << i;

        /* Wait out the remainder of Tslot */
        DELAY(60);   
    }
    while (++i < 8);

    return data;
} /* ds2411_read_byte */

/*
 * Byte 0:    8-bit family code (01h)
 * Bytes 1-6: 48-bit serial number
 * Byte 7:    8-bit CRC code
 */
int ds2411_read_id(struct ds2411_id *id)
{
    int level = disable_irq_save(); /* Timing sensitive */
    int i;
    unsigned char crc;

    /* Initialize delay factor based on loop time: 3*(uS-1) + 3 */
    ds2411_delay_factor = MAX(cpu_frequency / (1000000*3), 1);

    /* Init GPIO 1 wire bus for bit banging with a pullup resistor where 
     * it is set low as output and switched between input and output mode.
     * Required for bidirectional communication on a single wire.
     */
    or_l(DS2411_BIT, &GPIO_FUNCTION);   /* Set pin as GPIO            */
    and_l(~DS2411_BIT, &GPIO_ENABLE);   /* Set as input               */
    and_l(~DS2411_BIT, &GPIO_OUT);      /* Set low when set as output */

    /* Delay 100us to stabilize */
    DELAY(100);

    /* Issue reset pulse - 480uS or more to ensure standard (not overdrive)
       mode - we don't have the timing accuracy for that. */
    or_l(DS2411_BIT, &GPIO_ENABLE);
    /* Delay 560us: (Trstlmin + Trstlmax) / 2*/
    DELAY(560);
    and_l(~DS2411_BIT, &GPIO_ENABLE);
    /* Delay 66us: Tpdhmax + 6 */
    DELAY(66);

    /* Read presence pulse - line should be pulled low at proper time by the
       slave device */
    if ((GPIO_READ & DS2411_BIT) == 0)
    {
        /* Trsth + 1 - 66 = Tpdhmax + Tpdlmax + Trecmin + 1 - 66 */
        DELAY(240);

        /* ds2411 should be ready for data transfer */

        /* Send Read ROM command */
        ds2411_write_byte(0x33);

        /* Read ROM serial number and CRC */
        i = 0, crc = 0;

        do
        {
            unsigned char byte = ds2411_read_byte();
            ((unsigned char *)id)[i] = byte;
            crc = ds2411_calc_crc(crc ^ byte);
        }
        while (++i < 8);

        /* Check that family code is ok */
        if (id->family_code != 0x01)
        {
            logf("ds2411: invalid family code=%02X", (unsigned)id->family_code);
            i = DS2411_INVALID_FAMILY_CODE;
        }
        /* Check that CRC was ok */
        else if (crc != 0) /* Because last loop eors the CRC with the resulting CRC */
        {
            logf("ds2411: invalid CRC=%02X", (unsigned)id->crc);
            i = DS2411_INVALID_CRC;
        }
        else
        {
            /* Good ID read */
            i = DS2411_OK;
        }
    }
    else
    {
        logf("ds2411: no presence pulse");
        i = DS2411_NO_PRESENCE;
    }

    restore_irq(level);

    return i;
} /* ds2411_read_id */
