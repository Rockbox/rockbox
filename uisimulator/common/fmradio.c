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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "tuner.h"

#ifdef CONFIG_TUNER

static int frequency = 0;
static bool mono = false;

void radio_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            break;

        case RADIO_FREQUENCY:
            frequency = value;
            break;

        case RADIO_MUTE:
            break;

        case RADIO_FORCE_MONO:
            mono = value?true:false;
            break;

        default:
            return;
    }
}

int radio_get(int setting)
{
    int val = 0;
    
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

        case RADIO_ALL: /* debug query */
            break;
    }
    return val;
}

#endif
