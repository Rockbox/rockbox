/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner "middleware" for Philips TEA5760UK chip
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

#define I2C_ADR 0x22

/* define RSSI range */
#define RSSI_MIN 4
#define RSSI_MAX 46

static bool tuner_present = false;
static unsigned char write_bytes[7] = {
    0x00,   /* INTREG LSB */
    0x80,   /* FRQSET MSB */
    0x00,   /* FRQSET LSB */
    0x08,   /* TNCTRL MSB */
    0xD2,   /* TNCTRL LSB */
    0x00,   /* TESTREG MSB */
    0x40    /* TESTREG LSB */
};

static void tea5760_set_clear(int byte, unsigned char bits, int set)
{
    write_bytes[byte] &= ~bits;
    if (set)
        write_bytes[byte] |= bits;
}

/* tuner abstraction layer: set something to the tuner */
int tea5760_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            if (value) {
                /* sleep / standby mode */
                tea5760_set_clear(3, (1<<6), 0);
                tuner_power(false);
            }
            else {
                tuner_power(true);
                /* active mode */
                tea5760_set_clear(3, (1<<6), 1);
                /* disable hard mute */
                tea5760_set_clear(4, (1<<7), 0);
            }
            break;

        case RADIO_FREQUENCY:
            {
                int n;

                /* low side injection */
                tea5760_set_clear(4, (1<<4), 0);
                n = (4 * (value - 225000) + 16384) / 32768;

                /* set frequency in preset mode */
                write_bytes[1] = (n >> 8) & 0x3F;
                write_bytes[2] = n;
            }
            break;

        case RADIO_SCAN_FREQUENCY:
            tea5760_set(RADIO_FREQUENCY, value);
            sleep(40*HZ/1000);
            return tea5760_get(RADIO_TUNED);

        case RADIO_MUTE:
            tea5760_set_clear(3, (1<<2), value);
            break;

        case RADIO_REGION:
            {
            const struct fm_region_data *rd = &fm_region_data[value];
            int band = (rd->freq_min == 76000000) ? 1 : 0;
            int deemphasis = (rd->deemphasis == 50) ? 1 : 0;

            tea5760_set_clear(3, (1<<5), band);
            tea5760_set_clear(4, (1<<1), deemphasis);
            }
            break;
            
        case RADIO_FORCE_MONO:
            tea5760_set_clear(4, (1<<3), value);
            break;

        default:
            return -1;
    }

    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
    return 1;
}

/* tuner abstraction layer: read something from the tuner */
int tea5760_get(int setting)
{
    unsigned char read_bytes[16];
    int val = -1; /* default for unsupported query */

    fmradio_i2c_read(I2C_ADR, read_bytes, sizeof(read_bytes));

    switch(setting)
    {
        case RADIO_PRESENT:
            val = tuner_present ? 1 : 0;
            break;

        case RADIO_TUNED:
            val = 0;
            if (read_bytes[0] & (1<<4)) /* IF count correct */
            {
                val = read_bytes[8] >> 1; /* IF counter */
                val = (abs(val - 0x36) < 2); /* close match */
            }
            break;

        case RADIO_STEREO:
            val = read_bytes[9] >> 2;
            break;
            
        case RADIO_RSSI:
            val = (read_bytes[9] >> 4) & 0x0F;
            val = 4 + (28 * val + 5) / 10;
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

void tea5760_init(void)
{
    unsigned char buf[16];
    unsigned short manid, chipid;

    /* read all registers */
    fmradio_i2c_read(I2C_ADR, buf, sizeof(buf));
    
    /* check device id */
    manid = (buf[12] << 8) | buf[13];
    chipid = (buf[14] << 8) | buf[15];
    if ((manid == 0x202B) && (chipid == 0x5760))
    {
        tuner_present = true;
    }

    /* write initial values */
    tea5760_set_clear(3, (1<<1), 1);    /* soft mute on */
    tea5760_set_clear(3, (1<<0), 1);    /* stereo noise cancellation on */
    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
}

void tea5760_dbg_info(struct tea5760_dbg_info *info)
{
    fmradio_i2c_read(I2C_ADR, info->read_regs, sizeof(info->read_regs));
    memcpy(info->write_regs, write_bytes, sizeof(info->write_regs));
}

