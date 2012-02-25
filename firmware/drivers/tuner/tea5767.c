/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner "middleware" for Philips TEA5767 chip
 *
 * Copyright (C) 2004 Jörg Hohensohn
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
#include "power.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h"
#include "fmradio_i2c.h" /* physical interface driver */

#if defined(PHILIPS_HDD1630) || defined(ONDA_VX747) || defined(ONDA_VX777) || defined(PHILIPS_HDD6330)
#define I2C_ADR 0x60
#else
#define I2C_ADR 0xC0
#endif

/* define RSSI range */
#define RSSI_MIN 10
#define RSSI_MAX 55

static bool tuner_present = true;
static unsigned char write_bytes[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

static void tea5767_set_clear(int byte, unsigned char bits, int set)
{
    write_bytes[byte] &= ~bits;
    if (set)
        write_bytes[byte] |= bits;
}

/* tuner abstraction layer: set something to the tuner */
int tea5767_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            /* init values */
            write_bytes[0] |= (1<<7); /* mute */
#if CONFIG_TUNER_XTAL == 32768
            /* 32.768kHz, soft mute, stereo noise cancelling */
            write_bytes[3] |= (1<<4) | (1<<3) | (1<<1);
#else
            /* soft mute, stereo noise cancelling */
            write_bytes[3] |= (1<<3) | (1<<1); 
#endif
            /* sleep / standby mode */
            tea5767_set_clear(3, (1<<6), value);
            break;

        case RADIO_FREQUENCY:
            {
                int n;
#if CONFIG_TUNER_XTAL == 32768
                n = (4 * (value - 225000) + 16384) / 32768;
#else
                n = (4 * (value - 225000)) / 50000;
#endif
                write_bytes[0] = (write_bytes[0] & 0xC0) | (n >> 8);
                write_bytes[1] = n;
            }
            break;

        case RADIO_SCAN_FREQUENCY:
            tea5767_set(RADIO_FREQUENCY, value);
            sleep(HZ/30);
            return tea5767_get(RADIO_TUNED);

        case RADIO_MUTE:
            tea5767_set_clear(0, 0x80, value);
            break;

        case RADIO_REGION:
        {
            const struct fm_region_data *rd = &fm_region_data[value];
            int deemphasis = (rd->deemphasis == 75) ? 1 : 0;
            int band = (rd->freq_min == 76000000) ? 1 : 0;

            tea5767_set_clear(4, (1<<6), deemphasis);
            tea5767_set_clear(3, (1<<5), band);
            break;
        }
        case RADIO_FORCE_MONO:
            tea5767_set_clear(2, 0x08, value);
            break;
        default:
            return -1;
    }

    if(setting == RADIO_SLEEP && !value)
        tuner_power(true); /* wakeup */
    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
    if(setting == RADIO_SLEEP && value)
        tuner_power(false); /* sleep */
    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int tea5767_get(int setting)
{
    unsigned char read_bytes[5];
    int val = -1; /* default for unsupported query */

    fmradio_i2c_read(I2C_ADR, read_bytes, sizeof(read_bytes));

    switch(setting)
    {
        case RADIO_PRESENT:
            val = tuner_present;
            break;

        case RADIO_TUNED:
            val = 0;
            if (read_bytes[0] & 0x80) /* ready */
            {
                val = read_bytes[2] & 0x7F; /* IF counter */
                val = (abs(val - 0x36) < 2); /* close match */
            }
            break;

        case RADIO_STEREO:
            val = read_bytes[2] >> 7;
            break;
        
        case RADIO_RSSI:
            val = 10 + 3*(read_bytes[3] >> 4);
            break;

        case RADIO_RSSI_MIN:
            val = RSSI_MIN;
            break;

        case RADIO_RSSI_MAX:
            val = RSSI_MAX;
            break;
    }

    return val;
}

void tea5767_init(void)
{
/* save binsize by only detecting presence for targets where it may be absent */
#if defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330)
    unsigned char buf[5];
    unsigned char chipid;

    /* init chipid register with 0xFF in case fmradio_i2c_read fails silently */
    buf[3] = 0xFF;
    if (fmradio_i2c_read(I2C_ADR, buf, sizeof(buf)) < 0) {
        /* no i2c device detected */
        tuner_present = false;
    } else {
        /* check chip id */
        chipid = buf[3] & 0x0F;
        tuner_present = (chipid == 0);
    }
#endif
}

void tea5767_dbg_info(struct tea5767_dbg_info *info)
{
    fmradio_i2c_read(I2C_ADR, info->read_regs, 5);
    memcpy(info->write_regs, write_bytes, 5);
}
