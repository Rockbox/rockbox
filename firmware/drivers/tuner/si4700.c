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

/* I2C writes start at register 02h so the first two bytes are
   02h, next two 03h, etc. */
static unsigned char write_bytes[8]; /* registers 02 - 05 */
static bool tuner_present = false;

void si4700_init(void)
{
    unsigned char read_bytes[32];
    tuner_power(true);
    fmradio_i2c_read(I2C_ADR, read_bytes, sizeof(read_bytes));

    if ((read_bytes[12] << 8 | read_bytes[13]) == 0x1242)
    {
        tuner_present = true;
        /* fill in the initial values in write_bytes */
        memcpy(&write_bytes[0], &read_bytes[16], sizeof(write_bytes));
        /* -6dB volume, keep everything else as default */
        write_bytes[7] = (write_bytes[7] & ~0xf) | 0xc;
    }

    tuner_power(false);
}

static void si4700_tune(void)
{
    unsigned char read_bytes[1];

    write_bytes[2] |= (1 << 7); /* Set TUNE high to start tuning */
    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));

    do
    {
        sleep(HZ/50);
        fmradio_i2c_read(I2C_ADR, read_bytes, 1);
    }
    while (!(read_bytes[0] & (1 << 6))); /* STC high == Seek/Tune complete */

    write_bytes[2] &= ~(1 << 7); /* Set TUNE low */
    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
}

/* tuner abstraction layer: set something to the tuner */
int si4700_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            if (value)
            {
                write_bytes[1] = (1 | (1 << 6)); /* ENABLE high, DISABLE high */
            }
            else
            {
                write_bytes[1] = 1; /* ENABLE high, DISABLE low */
            }
            break;

        case RADIO_FREQUENCY:
        {
            static const unsigned int spacings[3] =
            {
                200000, 100000, 50000
            };
            unsigned int chan;
            unsigned int spacing = spacings[(write_bytes[7] >> 4) & 3] ;

            if (write_bytes[7] & (3 << 6)) /* check BAND */
            {
                chan = (value - 76000000) / spacing;
            }
            else
            {
                chan = (value - 87500000) / spacing;
            }

            write_bytes[2] = (write_bytes[2] & ~3) | ((chan & (3 << 8)) >> 8);
            write_bytes[3] = (chan & 0xff);
            fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
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
                write_bytes[0] &= ~(1 << 6);
            }
            else
            {
                /* unmute */
                write_bytes[0] |= (1 << 6);
            }
            break;

        case RADIO_REGION:
        {
            const struct si4700_region_data *rd =
                &si4700_region_data[value];

            write_bytes[4] = ((write_bytes[4] & ~(1 << 3)) | (rd->deemphasis << 3));
            write_bytes[7] = ((write_bytes[7] & ~(3 << 6)) | (rd->band << 6));
            write_bytes[7] = ((write_bytes[7] & ~(3 << 4)) | (rd->spacing << 4));
            break;
        }

        case RADIO_FORCE_MONO:
            if (value)
            {
                write_bytes[0] |= (1 << 5);
            }
            else
            {
                write_bytes[0] &= ~(1 << 5);
            }
            break;

        default:
            return -1;
    }

    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int si4700_get(int setting)
{
    /* I2C reads start with register 0xA */
    unsigned char read_bytes[1];
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
            fmradio_i2c_read(I2C_ADR, read_bytes, sizeof(read_bytes));
            val = (read_bytes[0] & 1); /* ST high == Stereo */
            break;
    }

    return val;
}

