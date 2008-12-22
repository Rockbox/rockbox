/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Tuner "middleware" for Silicon Labs SI4700 chip
 *
 * Copyright (C) 2008 Nils Wallménius
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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "kernel.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h"
#include "fmradio_i2c.h" /* physical interface driver */

#define I2C_ADR 0x20

#define DEVICEID    0x0
#define CHIPID      0x1
#define POWERCFG    0x2
#define CHANNEL     0x3
#define SYSCONFIG1  0x4
#define SYSCONFIG2  0x5
#define SYSCONFIG3  0x6
#define TEST1       0x7
#define TEST2       0x8
#define BOOTCONFIG  0x9
#define STATUSRSSI  0xA
#define READCHAN    0xB
#define RDSA        0xC
#define RDSB        0xD
#define RDSC        0xE
#define RDSD        0xF

/* some models use the internal 32 kHz oscillator which needs special attention
   during initialisation, power-up and power-down.
*/
#if defined(SANSA_CLIP) || defined(SANSA_E200V2) || defined(SANSA_FUZE)
#define USE_INTERNAL_OSCILLATOR
#endif

static bool tuner_present = false;
static unsigned short cache[16];

/* reads <len> registers from radio at offset 0x0A into cache */
static void si4700_read(int len)
{
    int i;
    unsigned char buf[32];
    unsigned char *ptr = buf;
    unsigned short data;
    
    fmradio_i2c_read(I2C_ADR, buf, len * 2);
    for (i = 0; i < len; i++) {
        data = ptr[0] << 8 | ptr[1];
        cache[(i + STATUSRSSI) & 0xF] = data;
        ptr += 2;
    }
}

/* writes <len> registers from cache to radio at offset 0x02 */
static void si4700_write(int len)
{
    int i;
    unsigned char buf[32];
    unsigned char *ptr = buf;
    unsigned short data;
    
    for (i = 0; i < len; i++) {
        data = cache[(i + POWERCFG) & 0xF];
        *ptr++ = (data >> 8) & 0xFF;
        *ptr++ = data & 0xFF;
    }
    fmradio_i2c_write(I2C_ADR, buf, len * 2);
}


void si4700_init(void)
{
    tuner_power(true);

    /* read all registers */
    si4700_read(16);
    
    /* check device id */
    if (cache[DEVICEID] == 0x1242)
    {
        tuner_present = true;

#ifdef USE_INTERNAL_OSCILLATOR
        /* enable the internal oscillator */
        cache[TEST1] |= (1 << 15);  /* XOSCEN */
        si4700_write(6);
        sleep(HZ/2);
#endif    
    }

    tuner_power(false);
}

static void si4700_tune(void)
{
    cache[CHANNEL] |= (1 << 15); /* Set TUNE high to start tuning */
    si4700_write(2);

    do
    {
        /* tuning should be done within 60 ms according to the datasheet */
        sleep(HZ * 60 / 1000);
        si4700_read(2);
    }
    while (!(cache[STATUSRSSI] & (1 << 14))); /* STC high */

    cache[CHANNEL] &= ~(1 << 15); /* Set TUNE low */
    si4700_write(2);
}

/* tuner abstraction layer: set something to the tuner */
int si4700_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            if (value)
            {
                /* power down */
                cache[POWERCFG] = (1 | (1 << 6)); /* ENABLE high, DISABLE high */
                si4700_write(1);
            }
            else
            {
                /* power up */
                cache[POWERCFG] = 1; /* ENABLE high, DISABLE low */
                si4700_write(1);
                sleep(110 * HZ / 1000);
                
                /* update register cache */
                si4700_read(16);               

                /* -6dB volume, keep everything else as default */
                cache[SYSCONFIG2] = (cache[SYSCONFIG2] & ~0xF) | 0xC;
                si4700_write(5);
            }
            return 1;

        case RADIO_FREQUENCY:
        {
            static const unsigned int spacings[3] =
            {
                200000, 100000, 50000
            };
            unsigned int chan;
            unsigned int spacing = spacings[(cache[5] >> 4) & 3] ;

            if (cache[SYSCONFIG2] & (3 << 6)) /* check BAND */
            {
                chan = (value - 76000000) / spacing;
            }
            else
            {
                chan = (value - 87500000) / spacing;
            }

            cache[CHANNEL] = (cache[CHANNEL] & ~0x3FF) | chan;
            si4700_tune();
            return 1;
        }

        case RADIO_SCAN_FREQUENCY:
            si4700_set(RADIO_FREQUENCY, value);
            return 1;

        case RADIO_MUTE:
            if (value)
            {
                /* mute */
                cache[POWERCFG] &= ~(1 << 14);
            }
            else
            {
                /* unmute */
                cache[POWERCFG] |= (1 << 14);
            }
            break;

        case RADIO_REGION:
        {
            const struct si4700_region_data *rd =
                &si4700_region_data[value];

            cache[SYSCONFIG1] = (cache[SYSCONFIG1] & ~(1 << 11)) | (rd->deemphasis << 11);
            cache[SYSCONFIG2] = (cache[SYSCONFIG2] & ~(3 << 6)) | (rd->band << 6);
            cache[SYSCONFIG2] = (cache[SYSCONFIG2] & ~(3 << 4)) | (rd->spacing << 4);
            break;
        }

        case RADIO_FORCE_MONO:
            if (value)
            {
                cache[POWERCFG] |= (1 << 13);
            }
            else
            {
                cache[POWERCFG] &= ~(1 << 13);
            }
            break;

        default:
            return -1;
    }

    si4700_write(5);
    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int si4700_get(int setting)
{
    int val = -1; /* default for unsupported query */

    switch(setting)
    {
        case RADIO_PRESENT:
            val = tuner_present ? 1 : 0;
            break;

        case RADIO_TUNED:
            val = 1;
            break;

        case RADIO_STEREO:
            si4700_read(1);
            val = (cache[STATUSRSSI] & (1 << 8)); /* ST high == Stereo */
            break;
    }

    return val;
}

