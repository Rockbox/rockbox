/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * General tuner functions
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
#include <stdlib.h>
#include "config.h"
#include "kernel.h"
#include "tuner.h"
#include "fmradio.h"

/* General region information */
const struct fm_region_data fm_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = { 87500000, 108000000, 100000, 50 },
    [REGION_US_CANADA] = { 87900000, 107900000, 200000, 75 },
    [REGION_JAPAN]     = { 76000000,  90000000, 100000, 50 },
    [REGION_KOREA]     = { 87500000, 108000000, 200000, 50 },
    [REGION_ITALY]     = { 87500000, 108000000,  50000, 50 },
    [REGION_OTHER]     = { 87500000, 108000000,  50000, 50 }
};

#ifndef SIMULATOR

#ifdef CONFIG_TUNER_MULTI
int (*tuner_set)(int setting, int value);
int (*tuner_get)(int setting);

#define TUNER_TYPE_CASE(type, set, get, ...)         \
    case type:                                       \
        tuner_set = set;                             \
        tuner_get = get;                             \
        __VA_ARGS__;                                 \
        break;
#else
#define TUNER_TYPE_CASE(type, set, get, ...)         \
        __VA_ARGS__;
#endif /* CONFIG_TUNER_MULTI */

void tuner_init(void)
{
#ifdef CONFIG_TUNER_MULTI
    switch (tuner_detect_type())
#endif
    {
    #if (CONFIG_TUNER & LV24020LP)
        TUNER_TYPE_CASE(LV24020LP,
                        lv24020lp_set,
                        lv24020lp_get,
                        lv24020lp_init())
    #endif
    #if (CONFIG_TUNER & TEA5760)
        TUNER_TYPE_CASE(TEA5760,
                        tea5760_set,
                        tea5760_get,
                        tea5760_init())
    #endif
    #if (CONFIG_TUNER & TEA5767)
        TUNER_TYPE_CASE(TEA5767,
                        tea5767_set,
                        tea5767_get,
                        tea5767_init())
    #endif
    #if (CONFIG_TUNER & S1A0903X01)
        TUNER_TYPE_CASE(S1A0903X01,
                        s1a0903x01_set,
                        s1a0903x01_get)
    #endif
    #if (CONFIG_TUNER & SI4700)
        TUNER_TYPE_CASE(SI4700,
                        si4700_set,
                        si4700_get,
                        si4700_init())
    #endif
    #if (CONFIG_TUNER & RDA5802)
        TUNER_TYPE_CASE(RDA5802,
                        rda5802_set,
                        rda5802_get,
                        rda5802_init())
    #endif
    #if (CONFIG_TUNER & STFM1000)
        TUNER_TYPE_CASE(STFM1000,
                        stfm1000_set,
                        stfm1000_get,
                        stfm1000_init())
    #endif
    }
}
#endif /* SIMULATOR */
