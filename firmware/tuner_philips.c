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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <string.h>
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio_i2c.h" /* physical interface driver */

#define I2C_ADR 0xC0
static unsigned char write_bytes[5];

/* tuner abstraction layer: set something to the tuner */
void philips_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_INIT:
            memset(write_bytes, 0, sizeof(write_bytes));
            break;

        case RADIO_FREQUENCY:
            {
                int n;
                n = (4 * (value - 225000)) / 50000;
                write_bytes[0] = (write_bytes[0] & 0xC0) | (n >> 8);
                write_bytes[1] = n & 0xFF;
            }
            break;

        case RADIO_MUTE:
            write_bytes[0] = (write_bytes[0] & 0x7F) | (value ? 0x80 : 0);
            break;

        case RADIO_FORCE_MONO:
            write_bytes[2] = (write_bytes[2] & 0xF7) | (value ? 0x08 : 0);
            fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
            break;

        case RADIO_IF_MEASUREMENT:
        case RADIO_SENSITIVITY:
        default:
            return;
    }
    fmradio_i2c_write(I2C_ADR, write_bytes, sizeof(write_bytes));
}

/* tuner abstraction layer: read something from the tuner */
int philips_get(int setting)
{
    unsigned char read_bytes[5];
    int val = -1; /* default for unsupported query */

    fmradio_i2c_read(I2C_ADR, read_bytes, sizeof(read_bytes));

    switch(setting)
    {
        case RADIO_PRESENT:
            val = 1; /* true */
            break;

        case RADIO_DEVIATION:
            val = read_bytes[2] & 0x7F;
            val = 222 - val*4; /* convert to kHz */
            break;

        case RADIO_STEREO:
            val = read_bytes[2] >> 7;
            break;
    }
    return val;
}
