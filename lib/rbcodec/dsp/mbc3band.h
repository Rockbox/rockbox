/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2024
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

#ifndef MBC3BAND_H
#define MBC3BAND_H

#include <stdbool.h>

/* Band indices */
#define MBC_BAND_LOW   0
#define MBC_BAND_MID   1
#define MBC_BAND_HIGH  2
#define MBC_NUM_BANDS  3

/* Operation modes — must match CHOICE_SETTING index (0-based) */
#define MBC_MODE_1BAND 0
#define MBC_MODE_2BAND 1
#define MBC_MODE_3BAND 2

struct ott_band_settings
{
    int threshold;      /* dB value, 0 = off, -60 to 0 */
    int ratio_down;     /* downward ratio: 200=2:1, 400=4:1, 1000=10:1, 100=limit (100*ratio) */
    int ratio_up;       /* upward ratio: 100=1:1, 50=0.5:1, 25=0.25:1 (100*ratio) */
    int attack_ms;      /* attack time in ms: 1-500 */
    int release_ms;     /* release time in ms: 10-2000 */
    int makeup_gain_db; /* makeup gain in 0.1 dB: 0 = off, 1-240 = 0.1-24.0 dB */
    int knee_db;        /* soft knee width: 0-12 dB */
    int wet_mix;        /* 0-100 percent */
};

struct mbc3band_settings
{
    bool enabled;
    int mode;                /* MBC_MODE_1BAND, MBC_MODE_2BAND, MBC_MODE_3BAND */
    int crossover_low;       /* Hz, low/mid split (default 120) */
    int crossover_high;      /* Hz, mid/high split (default 2000) */
    int output_gain;         /* -120 to +120 (0.1 dB steps, 0 = unity) */
    struct ott_band_settings bands[MBC_NUM_BANDS];
};

void dsp_set_mbc3band(const struct mbc3band_settings *settings);

#endif
