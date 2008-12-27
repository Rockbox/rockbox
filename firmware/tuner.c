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
    [REGION_EUROPE]    = { 87500000, 108000000,  50000 },
    [REGION_US_CANADA] = { 87900000, 107900000, 200000 },
    [REGION_JAPAN]     = { 76000000,  90000000, 100000 },
    [REGION_KOREA]     = { 87500000, 108000000, 100000 }
};

#ifndef SIMULATOR

/* Tuner-specific region information */

#if (CONFIG_TUNER & LV24020LP)
/* deemphasis setting for region */
const unsigned char lv24020lp_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = 0, /* 50uS */
    [REGION_US_CANADA] = 1, /* 75uS */
    [REGION_JAPAN]     = 0, /* 50uS */
    [REGION_KOREA]     = 0, /* 50uS */
};
#endif /* (CONFIG_TUNER & LV24020LP) */

#if (CONFIG_TUNER & TEA5767)
const struct tea5767_region_data tea5767_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = { 0, 0 }, /* 50uS, US/Europe band */
    [REGION_US_CANADA] = { 1, 0 }, /* 75uS, US/Europe band */
    [REGION_JAPAN]     = { 0, 1 }, /* 50uS, Japanese band  */
    [REGION_KOREA]     = { 0, 0 }, /* 50uS, US/Europe band */ 
};
#endif /* (CONFIG_TUNER & TEA5767) */

#if (CONFIG_TUNER & SI4700)
const struct si4700_region_data si4700_region_data[TUNER_NUM_REGIONS] =
{
    [REGION_EUROPE]    = { 1, 0, 2 }, /* 50uS, US/Europe band,  50kHz spacing */
    [REGION_US_CANADA] = { 0, 0, 0 }, /* 75uS, US/Europe band, 200kHz spacing */
    [REGION_JAPAN]     = { 1, 2, 1 }, /* 50uS, Japanese band,  100kHz spacing */
    [REGION_KOREA]     = { 1, 0, 1 }, /* 50uS, US/Europe band, 100kHz spacing */
};
#endif /* (CONFIG_TUNER & SI4700) */

#ifdef CONFIG_TUNER_MULTI
int (*tuner_set)(int setting, int value);
int (*tuner_get)(int setting);
#define TUNER_TYPE_CASE(type, set, get, ...) \
    case type:                                       \
        tuner_set = set;                             \
        tuner_get = get;                             \
        __VA_ARGS__;                                 \
        break;
#else
#define TUNER_TYPE_CASE(type, set, get, ...) \
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
    #if (CONFIG_TUNER & TEA5767)
        TUNER_TYPE_CASE(TEA5767,
                        tea5767_set,
                        tea5767_get)
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
    }
}

#endif /* SIMULATOR */
