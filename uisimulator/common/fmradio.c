/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "debug.h"
#include "tuner.h"
#ifdef HAVE_RDS_CAP
#include <strlcpy.h>
#include "system.h"
#include "rds.h"
#endif

#if CONFIG_TUNER

static int frequency = 0;
static bool mono = false;

static bool powered = false;

void tuner_init(void)
{
}

int tuner_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            break;

        case RADIO_FREQUENCY:
            frequency = value;
            break;

        case RADIO_SCAN_FREQUENCY:
            frequency = value;
            break;

        case RADIO_MUTE:
            break;

        case RADIO_FORCE_MONO:
            mono = (value != 0);
            break;

        default:
            return -1;
    }

    return 1;
}

int tuner_get(int setting)
{
    int val = 0;
#ifdef HAVE_RADIO_RSSI
    static int rssi = 0, rssidiff = 2;
#endif
    
    switch(setting)
    {
        case RADIO_PRESENT:
            val = 1; /* true */
            break;

        case RADIO_TUNED:
            if(frequency == 99500000)
                val = 1;
            break;

        case RADIO_STEREO:
            if(frequency == 99500000)
                val = mono?0:1;
            break;
            
#ifdef HAVE_RADIO_RSSI
        case RADIO_RSSI_MIN:
            val = 5;
            break;
        case RADIO_RSSI_MAX:
            val = 75;
            break;
        case RADIO_RSSI:
            rssi += rssidiff;
            if (rssi >= 75)
            {
                rssi = 75;
                rssidiff = -2;
            }
            else if (rssi < 5)
            {
                rssi = 5;
                rssidiff = 2;
            }
            val = rssi;
            break;
#endif

        case RADIO_ALL: /* debug query */
            break;
    }
    return val;
}

bool tuner_power(bool status)
{
    bool oldstatus = powered;
    powered = status;
    return oldstatus;
}

#ifdef HAVE_RDS_CAP
size_t tuner_get_rds_info(int setting, void *dst, size_t dstsize)
{
    /* TODO: integrate this into tuner_get/set */
    static const unsigned char info_id_tbl[] =
    {
        [RADIO_RDS_NAME]         = RDS_INFO_PS,
        [RADIO_RDS_TEXT]         = RDS_INFO_RT,
        [RADIO_RDS_PROGRAM_INFO] = RDS_INFO_PI,
        [RADIO_RDS_CURRENT_TIME] = RDS_INFO_CT,
    };

    if ((unsigned int)setting >= ARRAYLEN(info_id_tbl))
        return 0;

    switch (info_id_tbl[setting])
    {
    case RDS_INFO_PI:
        if (dstsize >= sizeof (uint16_t)) {
            *(uint16_t *)dst = 0;
        }
        dstsize = sizeof (uint16_t);
        break;
    case RDS_INFO_PS:
        dstsize = strlcpy(dst, "Rockbox Radio", dstsize);
        break;
    case RDS_INFO_RT:
        dstsize = strlcpy(dst, "http://www.rockbox.org", dstsize);
        break;
    case RDS_INFO_CT:
        if (dstsize >= sizeof (time_t)) {
            *(time_t *)dst = 0;
        }
        dstsize = sizeof (time_t);
        break;

    default:
        dstsize = 0;
    }

    return dstsize;
}
#endif /* HAVE_RDS_CAP */
#endif /* CONFIG_TUNER */
