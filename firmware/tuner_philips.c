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
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio_i2c.h" /* physical interface driver */

/* FIXME: this is just a dummy */

/* tuner abstraction layer: set something to the tuner */
void philips_set(int setting, int value)
{
    (void)value;
    switch(setting)
    {
        case RADIO_INIT:
            break;

        case RADIO_FREQUENCY:
            break;

        case RADIO_MUTE:
            break;

        case RADIO_IF_MEASUREMENT:
            break;

        case RADIO_SENSITIVITY:
            break;

        case RADIO_FORCE_MONO:
            break;
    }
}

/* tuner abstraction layer: read something from the tuner */
int philips_get(int setting)
{
    int val = -1;
    switch(setting)
    {
        case RADIO_PRESENT:
            val = 0; /* false */
            break;

        case RADIO_IF_MEASURED:
            break;

        case RADIO_STEREO:
            break;
    }
    return val;
}
